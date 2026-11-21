#ifndef DEVICETOOLBOX_MAINWINDOW_H
#define DEVICETOOLBOX_MAINWINDOW_H

#include "frameparser.h"
#include "modbusmaster.h"
#include "storage.h"

#include <QMainWindow>
#include <QVariantMap>

class QChart;
class QChartView;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QLineSeries;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTimer;
class QValueAxis;
class Device;
class DevicePanel;

// 功能：多协议调试台主窗口，负责选协议、开关设备、收发字节、Modbus 主站轮询与显示。
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // 功能：换协议时重建设备。
    void onTypeChanged(int index);
    // 功能：一个按钮管打开和关闭。
    void onOpenClicked();
    // 功能：把输入区内容转成字节发出去。
    void onSendClicked();
    // 功能：清空输出区和计数。
    void onClearClicked();
    // 功能：收到字节，切帧后显示。
    void onBytesRead(const QByteArray &bytes, const QString &flag);
    // 功能：发出字节后计数并显示。
    void onBytesWritten(const QByteArray &bytes, const QString &flag);
    // 功能：连上后锁参数、改按钮文字。
    void onOpened();
    // 功能：断开后解锁参数、改按钮文字。
    void onClosed();
    // 功能：把错误文字显示到状态栏。
    void onError(const QString &text);
    // 功能：点「开始轮询」时启动 Modbus 主站。
    void onPollButtonClicked();
    // 功能：定时器到点，发一帧读寄存器请求。
    void onPollTick();
    // 功能：拿到一次采样值，更新显示、曲线、落库。
    void onSample(qint64 tsMs, const QVariantMap &sample);
    // 功能：温度超阈值时变红提示。
    void onAlarm(const QString &text);

private:
    // 功能：按编号重建设备并接好统一信号。
    void rebuildDevice(int type);
    // 功能：把一帧按 HEX 或文本格式化后追加到输出区。
    void appendLine(const QString &dir, const QString &flag, const QByteArray &data);
    // 功能：把一帧写进 SQLite 并刷新记录条数。
    void storeFrame(const QString &dir, const QString &flag, const QByteArray &data);
    // 功能：刷新收发字节数标签。
    void updateCounters();
    // 功能：造 Modbus 主站参数与显示区。
    QWidget *makeModbusGroup();

    QComboBox     *m_typeCombo{nullptr};    // m_typeCombo：设备类型下拉框
    QPushButton   *m_openButton{nullptr};   // m_openButton：打开/关闭两用按钮
    QLabel        *m_status{nullptr};       // m_status：顶部状态文字
    DevicePanel   *m_panel{nullptr};        // m_panel：参数面板
    QComboBox     *m_frameMode{nullptr};    // m_frameMode：帧切法下拉框
    QCheckBox     *m_hexDisplay{nullptr};   // m_hexDisplay：接收区按 HEX 显示
    QCheckBox     *m_hexInput{nullptr};     // m_hexInput：发送区按 HEX 输入
    QCheckBox     *m_logEnabled{nullptr};   // m_logEnabled：是否把收发写进 SQLite
    QLabel        *m_recordLabel{nullptr};  // m_recordLabel：已落库记录条数
    QPlainTextEdit *m_output{nullptr};      // m_output：接收区
    QPlainTextEdit *m_input{nullptr};       // m_input：发送区
    QPushButton   *m_sendButton{nullptr};   // m_sendButton：发送按钮
    QPushButton   *m_clearButton{nullptr};  // m_clearButton：清空按钮
    QLabel        *m_rxLabel{nullptr};      // m_rxLabel：接收字节数
    QLabel        *m_txLabel{nullptr};      // m_txLabel：发送字节数

    QSpinBox      *m_slaveId{nullptr};      // m_slaveId：从站号
    QSpinBox      *m_startAddr{nullptr};    // m_startAddr：起始寄存器地址
    QSpinBox      *m_regCount{nullptr};     // m_regCount：读几个寄存器
    QSpinBox      *m_interval{nullptr};     // m_interval：轮询间隔毫秒
    QDoubleSpinBox *m_tempLimit{nullptr};   // m_tempLimit：温度告警阈值
    QPushButton   *m_pollButton{nullptr};   // m_pollButton：开始/停止轮询
    QLabel        *m_tempLabel{nullptr};    // m_tempLabel：温度显示
    QLabel        *m_currentLabel{nullptr}; // m_currentLabel：电流显示
    QLabel        *m_voltageLabel{nullptr}; // m_voltageLabel：电压显示
    QLabel        *m_devStatusLabel{nullptr};// m_devStatusLabel：设备状态显示
    QLabel        *m_sampleLabel{nullptr};  // m_sampleLabel：采样条数
    QChartView    *m_chartView{nullptr};    // m_chartView：温度曲线
    QLineSeries   *m_tempSeries{nullptr};   // m_tempSeries：温度曲线数据
    QChart        *m_chart{nullptr};        // m_chart：图表对象
    QValueAxis    *m_axisX{nullptr};        // m_axisX：时间轴
    QValueAxis    *m_axisY{nullptr};        // m_axisY：温度轴

    Device       *m_device{nullptr};        // m_device：当前通信对象
    FrameParser   m_parser;                 // m_parser：帧切分器
    ModbusMaster  m_master;                 // m_master：Modbus 协议层
    Storage       m_storage;                // m_storage：收发记录与采样落库
    QTimer       *m_pollTimer{nullptr};     // m_pollTimer：轮询定时器
    bool          m_running{false};         // m_running：设备是否开着
    bool          m_polling{false};         // m_polling：是否在轮询
    quint64       m_rxBytes{0};             // m_rxBytes：累计接收字节
    quint64       m_txBytes{0};             // m_txBytes：累计发送字节
    quint64       m_recordCount{0};         // m_recordCount：已落库收发记录条数
    quint64       m_sampleCount{0};         // m_sampleCount：已落库采样条数
    qint64        m_t0{0};                  // m_t0：曲线时间原点
};

#endif // DEVICETOOLBOX_MAINWINDOW_H
