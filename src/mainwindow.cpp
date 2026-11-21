#include "mainwindow.h"
#include "devicepanel.h"
#include "devices.h"

#include <QChart>
#include <QChartView>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLineSeries>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <QValueAxis>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);

    auto *top = new QHBoxLayout();
    m_typeCombo = new QComboBox(central);
    m_typeCombo->addItem("TCP 客户端", TcpClient);
    m_typeCombo->addItem("TCP 服务端", TcpServer);
    m_typeCombo->addItem("UDP", Udp);
    m_typeCombo->addItem("串口", SerialPort);
    m_openButton = new QPushButton("打开设备", central);
    m_status = new QLabel("未连接", central);
    top->addWidget(new QLabel("设备类型", central));
    top->addWidget(m_typeCombo);
    top->addWidget(m_openButton);
    top->addWidget(m_status, 1);
    root->addLayout(top);

    auto *body = new QHBoxLayout();
    auto *left = new QVBoxLayout();
    auto *paramGroup = new QGroupBox("设备参数", central);
    auto *paramLayout = new QVBoxLayout(paramGroup);
    m_panel = new DevicePanel(paramGroup);
    paramLayout->addWidget(m_panel);
    paramLayout->addStretch(1);

    auto *formatGroup = new QGroupBox("显示与帧格式", central);
    auto *formatLayout = new QFormLayout(formatGroup);
    m_frameMode = new QComboBox(formatGroup);
    m_frameMode->addItem("原始字节", 0);
    m_frameMode->addItem("按行切帧", 1);
    m_frameMode->addItem("Modbus TCP 按长度切", 2);
    m_hexDisplay = new QCheckBox("接收区按 HEX 显示", formatGroup);
    m_hexInput = new QCheckBox("发送区按 HEX 输入", formatGroup);
    m_logEnabled = new QCheckBox("记录收发到 SQLite", formatGroup);
    m_logEnabled->setChecked(true);
    formatLayout->addRow("帧格式", m_frameMode);
    formatLayout->addWidget(m_hexDisplay);
    formatLayout->addWidget(m_hexInput);
    formatLayout->addWidget(m_logEnabled);

    left->addWidget(paramGroup);
    left->addWidget(formatGroup);
    left->addStretch(1);
    body->addLayout(left);

    auto *right = new QVBoxLayout();
    auto *rxBar = new QHBoxLayout();
    m_rxLabel = new QLabel("RX 0 B", central);
    m_recordLabel = new QLabel("已记录 0 条", central);
    rxBar->addWidget(new QLabel("接收区", central));
    rxBar->addWidget(m_rxLabel, 1);
    rxBar->addWidget(m_recordLabel);
    right->addLayout(rxBar);

    m_output = new QPlainTextEdit(central);
    m_output->setReadOnly(true);
    m_output->setMaximumBlockCount(2000);
    m_output->setLineWrapMode(QPlainTextEdit::NoWrap);
    right->addWidget(m_output, 2);

    auto *txBar = new QHBoxLayout();
    m_txLabel = new QLabel("TX 0 B", central);
    txBar->addWidget(new QLabel("发送区", central));
    txBar->addWidget(m_txLabel, 1);
    right->addLayout(txBar);

    m_input = new QPlainTextEdit(central);
    m_input->setMaximumHeight(70);
    m_input->setPlainText("READ");
    right->addWidget(m_input, 1);

    auto *buttons = new QHBoxLayout();
    m_sendButton = new QPushButton("发送", central);
    m_clearButton = new QPushButton("清空", central);
    buttons->addWidget(m_sendButton);
    buttons->addWidget(m_clearButton);
    buttons->addStretch(1);
    right->addLayout(buttons);

    right->addWidget(makeModbusGroup(), 6);
    body->addLayout(right);
    body->setStretch(0, 1);
    body->setStretch(1, 3);
    root->addLayout(body);

    setCentralWidget(central);
    setWindowTitle("DeviceToolbox · 多协议设备调试台 + Modbus 主站");
    resize(1280, 900);
    statusBar()->showMessage("先启动假设备，再点「打开设备」");

    connect(m_typeCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onTypeChanged);
    connect(m_openButton, &QPushButton::clicked, this, &MainWindow::onOpenClicked);
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(m_frameMode, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_parser.setMode(static_cast<FrameParser::Mode>(index));
    });
    connect(m_pollButton, &QPushButton::clicked, this, &MainWindow::onPollButtonClicked);
    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::onPollTick);
    connect(&m_master, &ModbusMaster::sampleReady, this, &MainWindow::onSample);
    connect(&m_master, &ModbusMaster::alarm, this, &MainWindow::onAlarm);
    connect(&m_master, &ModbusMaster::protocolError, this, &MainWindow::onError);

    QString err;
    if (!m_storage.open("records.db", err)) {
        m_status->setText("数据库打开失败：" + err);
        m_logEnabled->setChecked(false);
        m_logEnabled->setEnabled(false);
    } else {
        int n = m_storage.recordCount();
        m_recordCount = n < 0 ? 0 : quint64(n);
        m_recordLabel->setText(QString("已记录 %1 条").arg(m_recordCount));
        int s = m_storage.sampleCount();
        m_sampleCount = s < 0 ? 0 : quint64(s);
        m_sampleLabel->setText(QString("采样 %1 条").arg(m_sampleCount));
    }

    rebuildDevice(m_typeCombo->currentIndex());
}

MainWindow::~MainWindow()
{
    if (m_pollTimer) m_pollTimer->stop();
    if (m_device) m_device->closeDevice();
    m_storage.close();
}

// 功能：造 Modbus 主站参数区、实时值显示和温度曲线。
QWidget *MainWindow::makeModbusGroup()
{
    auto *group = new QGroupBox("Modbus 主站（TCP）· 读保持寄存器", this);
    auto *layout = new QVBoxLayout(group);

    auto *form = new QFormLayout();
    m_slaveId = new QSpinBox(group);
    m_slaveId->setRange(1, 247);
    m_slaveId->setValue(1);
    m_startAddr = new QSpinBox(group);
    m_startAddr->setRange(0, 65535);
    m_startAddr->setValue(0);
    m_regCount = new QSpinBox(group);
    m_regCount->setRange(4, 16);
    m_regCount->setValue(4);
    m_interval = new QSpinBox(group);
    m_interval->setRange(100, 5000);
    m_interval->setValue(500);
    m_interval->setSuffix(" ms");
    m_tempLimit = new QDoubleSpinBox(group);
    m_tempLimit->setRange(0, 200);
    m_tempLimit->setValue(30.0);
    m_tempLimit->setSuffix(" °C");
    form->addRow("从站号", m_slaveId);
    form->addRow("起始地址", m_startAddr);
    form->addRow("读几个寄存器", m_regCount);
    form->addRow("轮询间隔", m_interval);
    form->addRow("温度告警阈值", m_tempLimit);
    layout->addLayout(form);

    m_pollButton = new QPushButton("开始轮询", group);
    m_tempLabel = new QLabel("温度 --", group);
    m_currentLabel = new QLabel("电流 --", group);
    m_voltageLabel = new QLabel("电压 --", group);
    m_devStatusLabel = new QLabel("状态 --", group);
    m_sampleLabel = new QLabel("采样 0 条", group);
    auto *values = new QHBoxLayout();
    values->addWidget(m_pollButton);
    values->addWidget(m_tempLabel);
    values->addWidget(m_currentLabel);
    values->addWidget(m_voltageLabel);
    values->addWidget(m_devStatusLabel);
    values->addWidget(m_sampleLabel);
    values->addStretch(1);
    layout->addLayout(values);

    m_chart = new QChart();
    m_tempSeries = new QLineSeries();
    m_chart->addSeries(m_tempSeries);
    m_chart->setTitle("温度实时曲线");
    m_chart->legend()->hide();
    m_axisX = new QValueAxis();
    m_axisX->setRange(0, 120);
    m_axisX->setTitleText("秒");
    m_axisX->setLabelFormat("%.0f");
    m_axisX->setTickCount(7);
    m_axisY = new QValueAxis();
    m_axisY->setRange(20, 40);
    m_axisY->setTitleText("°C");
    m_axisY->setLabelFormat("%.0f");
    m_axisY->setTickCount(6);
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_tempSeries->attachAxis(m_axisX);
    m_tempSeries->attachAxis(m_axisY);
    m_chartView = new QChartView(m_chart, group);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(240);
    layout->addWidget(m_chartView, 3);

    m_pollTimer = new QTimer(this);
    return group;
}

// 功能：按编号重建设备并接好统一信号。
void MainWindow::rebuildDevice(int type)
{
    if (m_polling) {
        m_polling = false;
        m_pollTimer->stop();
        m_pollButton->setText("开始轮询");
    }

    if (m_device) {
        m_device->closeDevice();
        delete m_device;
        m_device = nullptr;
    }

    m_device = createDevice(type, this);
    if (!m_device) return;

    connect(m_device, &Device::bytesRead, this, &MainWindow::onBytesRead);
    connect(m_device, &Device::bytesWritten, this, &MainWindow::onBytesWritten);
    connect(m_device, &Device::opened, this, &MainWindow::onOpened);
    connect(m_device, &Device::closed, this, &MainWindow::onClosed);
    connect(m_device, &Device::errorOccurred, this, &MainWindow::onError);

    m_panel->setType(type);
    m_parser.reset();
    onClosed();
}

// 功能：换协议时重建设备。
void MainWindow::onTypeChanged(int index)
{
    rebuildDevice(index);
}

// 功能：一个按钮管打开和关闭。
void MainWindow::onOpenClicked()
{
    if (!m_device) return;
    m_openButton->setEnabled(false);
    if (m_running) {
        m_device->closeDevice();
        return;
    }
    m_status->setText("正在连接…");
    m_device->openDevice(m_panel->save());
}

// 功能：把输入区内容转成字节发出去。
void MainWindow::onSendClicked()
{
    if (!m_running) {
        m_status->setText("请先打开设备");
        return;
    }
    QString text = m_input->toPlainText();
    QByteArray bytes = m_hexInput->isChecked()
                       ? QByteArray::fromHex(text.toLatin1())
                       : text.toUtf8();
    if (bytes.isEmpty()) {
        m_status->setText("输入为空或 HEX 不合法");
        return;
    }
    m_device->writeBytes(bytes);
}

// 功能：清空输出区和计数。
void MainWindow::onClearClicked()
{
    m_output->clear();
    m_rxBytes = 0;
    m_txBytes = 0;
    m_parser.reset();
    updateCounters();
}

// 功能：收到字节，切帧后显示；轮询中把每帧交给 Modbus 协议层。
void MainWindow::onBytesRead(const QByteArray &bytes, const QString &flag)
{
    m_rxBytes += quint64(bytes.size());
    const QList<QByteArray> frames = m_parser.feed(bytes);
    for (const QByteArray &one : frames) {
        appendLine("RX", flag, one);
        storeFrame("RX", flag, one);
        if (m_polling) m_master.onFrame(one);
    }
    updateCounters();
}

// 功能：发出字节后计数并显示。
void MainWindow::onBytesWritten(const QByteArray &bytes, const QString &flag)
{
    m_txBytes += quint64(bytes.size());
    appendLine("TX", flag, bytes);
    storeFrame("TX", flag, bytes);
    updateCounters();
}

// 功能：点「开始轮询」时启动或停止 Modbus 主站。
void MainWindow::onPollButtonClicked()
{
    if (!m_polling) {
        if (!m_running) {
            m_status->setText("请先打开设备");
            return;
        }
        if (m_typeCombo->currentIndex() != TcpClient) {
            m_status->setText("Modbus 主站请用 TCP 客户端模式");
            return;
        }
        m_frameMode->setCurrentIndex(2);
        m_master.setTempLimit(m_tempLimit->value());
        m_polling = true;
        m_pollButton->setText("停止轮询");
        m_t0 = QDateTime::currentMSecsSinceEpoch();
        m_tempSeries->clear();
        m_axisX->setRange(0, 120);
        m_pollTimer->start(m_interval->value());
        onPollTick();
        m_status->setText("Modbus 轮询中");
    } else {
        m_polling = false;
        m_pollTimer->stop();
        m_pollButton->setText("开始轮询");
        m_status->setText("轮询已停止");
    }
}

// 功能：定时器到点，发一帧读寄存器请求。
void MainWindow::onPollTick()
{
    if (!m_running || !m_polling || !m_device) return;
    QByteArray request = m_master.buildReadRequest(
                quint8(m_slaveId->value()),
                quint16(m_startAddr->value()),
                quint16(m_regCount->value()));
    m_device->writeBytes(request);
}

// 功能：拿到一次采样值，更新显示、曲线、落库。
void MainWindow::onSample(qint64 tsMs, const QVariantMap &sample)
{
    double temp = sample.value("temp").toDouble();
    double current = sample.value("current").toDouble();
    double voltage = sample.value("voltage").toDouble();
    int status = sample.value("status").toInt();

    m_tempLabel->setText(QString("温度 %1 °C").arg(temp, 0, 'f', 1));
    m_currentLabel->setText(QString("电流 %1 A").arg(current, 0, 'f', 1));
    m_voltageLabel->setText(QString("电压 %1 V").arg(voltage, 0, 'f', 1));
    m_devStatusLabel->setText(QString("状态 %1").arg(status));
    m_tempLabel->setStyleSheet("");

    double x = (tsMs - m_t0) / 1000.0;
    m_tempSeries->append(x, temp);
    if (x > 120) m_axisX->setRange(x - 120, x);

    if (m_storage.insertSample(tsMs, temp, current, voltage, status)) {
        ++m_sampleCount;
        m_sampleLabel->setText(QString("采样 %1 条").arg(m_sampleCount));
    }
}

// 功能：温度超阈值时变红提示。
void MainWindow::onAlarm(const QString &text)
{
    m_tempLabel->setStyleSheet("color: #c0392b; font-weight: bold;");
    m_output->appendPlainText("[ALARM] " + text);
    statusBar()->showMessage(text, 5000);
}

// 功能：把一帧写进 SQLite 并刷新记录条数。
void MainWindow::storeFrame(const QString &dir, const QString &flag, const QByteArray &data)
{
    if (!m_logEnabled->isChecked()) return;

    qint64 ts = QDateTime::currentMSecsSinceEpoch();
    if (m_storage.log(ts, dir, m_typeCombo->currentText(), flag, data)) {
        ++m_recordCount;
        m_recordLabel->setText(QString("已记录 %1 条").arg(m_recordCount));
    }
}

// 功能：把一帧按 HEX 或文本格式化后追加到输出区。
void MainWindow::appendLine(const QString &dir, const QString &flag, const QByteArray &data)
{
    QString text = m_hexDisplay->isChecked()
                   ? QString::fromLatin1(data.toHex(' '))
                   : QString::fromUtf8(data).trimmed();
    if (text.isEmpty()) return;

    m_output->appendPlainText(QString("[%1] %2  %3  %4")
                                      .arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz"))
                                      .arg(dir)
                                      .arg(flag)
                                      .arg(text));
}

// 功能：刷新收发字节数标签。
void MainWindow::updateCounters()
{
    m_rxLabel->setText(QString("RX %1 B").arg(m_rxBytes));
    m_txLabel->setText(QString("TX %1 B").arg(m_txBytes));
}

// 功能：连上后锁参数、改按钮文字。
void MainWindow::onOpened()
{
    m_running = true;
    m_openButton->setEnabled(true);
    m_openButton->setText("关闭设备");
    m_panel->setEnabled(false);
    m_typeCombo->setEnabled(false);
    m_status->setText("已连接");
}

// 功能：断开后解锁参数、改按钮文字，并停掉轮询。
void MainWindow::onClosed()
{
    m_running = false;
    if (m_polling) {
        m_polling = false;
        m_pollTimer->stop();
        m_pollButton->setText("开始轮询");
    }
    m_openButton->setEnabled(true);
    m_openButton->setText("打开设备");
    m_panel->setEnabled(true);
    m_typeCombo->setEnabled(true);
    if (m_status) m_status->setText("未连接");
}

// 功能：把错误文字显示到状态栏。
void MainWindow::onError(const QString &text)
{
    m_status->setText("错误：" + text);
    statusBar()->showMessage(text, 5000);
}
