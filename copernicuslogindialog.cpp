#include "copernicuslogindialog.h"

// Константы для диалогового окна входа
namespace LoginDialogConstants 
{
constexpr int MINIMUM_WIDTH = 300;
inline const QString WINDOW_TITLE = "Вход в Copernicus Data Space Ecosystem API";
inline const QString LOGIN_LABEL_TEXT = "Логин:";
inline const QString PASSWORD_LABEL_TEXT = "Пароль:";
inline const QString LINK_TEXT = "Перейти на сайт Copernicus для регистрации";
inline const QString COPERNICUS_URL = "https://dataspace.copernicus.eu/";
}

CopernicusLoginDialog::CopernicusLoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setupWindow();
    createWidgets();
    setupLayout();
    setupConnections();
}

void CopernicusLoginDialog::setupWindow()
{
    setWindowTitle(LoginDialogConstants::WINDOW_TITLE);
    setMinimumWidth(LoginDialogConstants::MINIMUM_WIDTH);
}

void CopernicusLoginDialog::createWidgets()
{
    m_loginEdit = new QLineEdit();
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
}

void CopernicusLoginDialog::setupLayout()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Логин
    QLabel *loginLabel = new QLabel(LoginDialogConstants::LOGIN_LABEL_TEXT);
    mainLayout->addWidget(loginLabel);
    mainLayout->addWidget(m_loginEdit);
    
    // Пароль
    QLabel *passwordLabel = new QLabel(LoginDialogConstants::PASSWORD_LABEL_TEXT);
    mainLayout->addWidget(passwordLabel);
    mainLayout->addWidget(m_passwordEdit);
    
    // Ссылка на сайт
    QLabel *linkLabel = new QLabel();
    QString linkHtml = QString("<a href=\"%1\">%2</a>").arg(LoginDialogConstants::COPERNICUS_URL, LoginDialogConstants::LINK_TEXT);
    linkLabel->setText(linkHtml);
    linkLabel->setOpenExternalLinks(true);
    linkLabel->setTextFormat(Qt::RichText);
    mainLayout->addWidget(linkLabel);
    
    // Кнопки
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addStretch();
    mainLayout->addWidget(buttonBox);
    
    setLayout(mainLayout);
}

void CopernicusLoginDialog::setupConnections()
{
    QDialogButtonBox *buttonBox = findChild<QDialogButtonBox*>();
    if (buttonBox)
    {
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }
}

QString CopernicusLoginDialog::getLogin() const
{
    return m_loginEdit->text();
}

QString CopernicusLoginDialog::getPassword() const
{
    return m_passwordEdit->text();
}

void CopernicusLoginDialog::setLogin(const QString &login)
{
    m_loginEdit->setText(login);
}

void CopernicusLoginDialog::setPassword(const QString &password)
{
    m_passwordEdit->setText(password);
}
