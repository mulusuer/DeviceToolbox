#ifndef DEVICETOOLBOX_DEVICEPANEL_H
#define DEVICETOOLBOX_DEVICEPANEL_H

#include <QVariantMap>
#include <QWidget>

class QComboBox;
class QLineEdit;
class QSpinBox;
class QStackedWidget;

// 功能：按设备类型切换参数页，并把当前页的控件值打包成参数。
class DevicePanel : public QWidget
{
    Q_OBJECT
public:
    explicit DevicePanel(QWidget *parent = nullptr);

    // 功能：切到指定类型的参数页。
    void setType(int type);
    // 功能：把当前页的控件值打包成参数。
    QVariantMap save() const;

private:
    // 功能：造出四种协议各自的参数页。
    QWidget *makeTcpClientPage();
    QWidget *makeTcpServerPage();
    QWidget *makeUdpPage();
    QWidget *makeSerialPage();

    QStackedWidget *m_stack{nullptr};        // m_stack：四页参数面板

    QLineEdit *m_clientHost{nullptr};        // m_clientHost：TCP 客户端目标地址
    QSpinBox  *m_clientPort{nullptr};       // m_clientPort：TCP 客户端目标端口
    QSpinBox  *m_serverPort{nullptr};       // m_serverPort：TCP 服务端监听端口
    QSpinBox  *m_udpLocalPort{nullptr};     // m_udpLocalPort：UDP 本地绑定端口
    QLineEdit *m_udpRemoteHost{nullptr};    // m_udpRemoteHost：UDP 目标地址
    QSpinBox  *m_udpRemotePort{nullptr};    // m_udpRemotePort：UDP 目标端口

    QComboBox *m_serialPort{nullptr};       // m_serialPort：串口号
    QComboBox *m_baudRate{nullptr};         // m_baudRate：波特率
    QComboBox *m_dataBits{nullptr};         // m_dataBits：数据位
    QComboBox *m_parity{nullptr};           // m_parity：校验位
    QComboBox *m_stopBits{nullptr};         // m_stopBits：停止位
    QComboBox *m_flowControl{nullptr};     // m_flowControl：流控
};

#endif // DEVICETOOLBOX_DEVICEPANEL_H
