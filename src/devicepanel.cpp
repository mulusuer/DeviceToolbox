#include "devicepanel.h"
#include "devices.h"

#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSerialPortInfo>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

// 功能：摆一个两列的参数表单页。
static QFormLayout *makeForm(QWidget *page)
{
    auto *form = new QFormLayout(page);      // form：左边标签右边控件的布局
    form->setContentsMargins(6, 6, 6, 6);
    return form;
}

// 功能：造一个 0~65535 的端口输入框。
static QSpinBox *makePortBox(QWidget *parent, int value)
{
    auto *box = new QSpinBox(parent);        // box：端口输入框
    box->setRange(0, 65535);
    box->setValue(value);
    return box;
}

DevicePanel::DevicePanel(QWidget *parent)
    : QWidget(parent)
{
    m_stack = new QStackedWidget(this);
    m_stack->addWidget(makeTcpClientPage());
    m_stack->addWidget(makeTcpServerPage());
    m_stack->addWidget(makeUdpPage());
    m_stack->addWidget(makeSerialPage());

    auto *outer = new QVBoxLayout(this);     // outer：面板自身布局
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(m_stack);
}

// 功能：切到指定类型的参数页。
void DevicePanel::setType(int type)
{
    m_stack->setCurrentIndex(type);
}

// 功能：把当前页的控件值打包成参数。
QVariantMap DevicePanel::save() const
{
    QVariantMap p;                           // p：交给设备层的参数包
    switch (m_stack->currentIndex()) {
    case TcpClient:
        p["host"] = m_clientHost->text();
        p["port"] = m_clientPort->value();
        break;
    case TcpServer:
        p["port"] = m_serverPort->value();
        break;
    case Udp:
        p["localPort"] = m_udpLocalPort->value();
        p["host"] = m_udpRemoteHost->text();
        p["port"] = m_udpRemotePort->value();
        break;
    case SerialPort:
        p["portName"] = m_serialPort->currentText();
        p["baudRate"] = m_baudRate->currentData().toInt();
        p["dataBits"] = m_dataBits->currentData().toInt();
        p["parity"] = m_parity->currentData().toInt();
        p["stopBits"] = m_stopBits->currentData().toInt();
        p["flowControl"] = m_flowControl->currentData().toInt();
        break;
    default:
        break;
    }
    return p;
}

// 功能：造 TCP 客户端参数页。
QWidget *DevicePanel::makeTcpClientPage()
{
    auto *page = new QWidget(this);
    auto *form = makeForm(page);
    m_clientHost = new QLineEdit("127.0.0.1", page);
    m_clientPort = makePortBox(page, 1502);
    form->addRow("设备地址", m_clientHost);
    form->addRow("设备端口", m_clientPort);
    return page;
}

// 功能：造 TCP 服务端参数页。
QWidget *DevicePanel::makeTcpServerPage()
{
    auto *page = new QWidget(this);
    auto *form = makeForm(page);
    m_serverPort = makePortBox(page, 1503);
    form->addRow("监听端口", m_serverPort);
    return page;
}

// 功能：造 UDP 参数页。
QWidget *DevicePanel::makeUdpPage()
{
    auto *page = new QWidget(this);
    auto *form = makeForm(page);
    m_udpLocalPort = makePortBox(page, 1504);
    m_udpRemoteHost = new QLineEdit("127.0.0.1", page);
    m_udpRemotePort = makePortBox(page, 1502);
    form->addRow("本地端口", m_udpLocalPort);
    form->addRow("目标地址", m_udpRemoteHost);
    form->addRow("目标端口", m_udpRemotePort);
    return page;
}

// 功能：造串口参数页。
QWidget *DevicePanel::makeSerialPage()
{
    auto *page = new QWidget(this);
    auto *form = makeForm(page);

    m_serialPort = new QComboBox(page);
    m_serialPort->setEditable(true);
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        m_serialPort->addItem(info.portName());
    }
    if (m_serialPort->count() == 0) m_serialPort->addItem("COM3");

    m_baudRate = new QComboBox(page);
    for (int v : {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200}) {
        m_baudRate->addItem(QString::number(v), v);
    }
    m_baudRate->setCurrentIndex(m_baudRate->findData(9600));

    m_dataBits = new QComboBox(page);
    m_dataBits->addItem("8", 8);
    m_dataBits->addItem("7", 7);
    m_dataBits->addItem("6", 6);
    m_dataBits->addItem("5", 5);

    m_parity = new QComboBox(page);
    m_parity->addItem("None", 0);
    m_parity->addItem("Even", 2);
    m_parity->addItem("Odd", 3);
    m_parity->addItem("Space", 4);
    m_parity->addItem("Mark", 5);

    m_stopBits = new QComboBox(page);
    m_stopBits->addItem("1", 1);
    m_stopBits->addItem("2", 2);
    m_stopBits->addItem("1.5", 3);

    m_flowControl = new QComboBox(page);
    m_flowControl->addItem("None", 0);
    m_flowControl->addItem("Hardware", 1);
    m_flowControl->addItem("Software", 2);

    form->addRow("串口号", m_serialPort);
    form->addRow("波特率", m_baudRate);
    form->addRow("数据位", m_dataBits);
    form->addRow("校验位", m_parity);
    form->addRow("停止位", m_stopBits);
    form->addRow("流控", m_flowControl);
    return page;
}
