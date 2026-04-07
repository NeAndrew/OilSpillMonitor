#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>

/**
 * @brief Класс диалогового окна аутентификации в Copernicus Data Space Ecosystem API
 *
 * Предоставляет пользовательский интерфейс для ввода логина и пароля
 * с возможностью перехода на сайт для регистрации.
 */
class CopernicusLoginDialog : public QDialog
{
    Q_OBJECT

public:
    // Конструктор
    explicit CopernicusLoginDialog(QWidget *parent = nullptr);

    /**
     * @brief Геттер получения введенного логина
     * @return Текст логина
     */
    QString getLogin() const;
    
    /**
     * @brief Геттер получения введенного пароля
     * @return Текст пароля
     */
    QString getPassword() const;
    
    /**
     * @brief Сеттер установки логина в поле ввода
     * @param login Логин для установки
     */
    void setLogin(const QString &login);
    
    /**
     * @brief Сеттер установки пароля в поле ввода
     * @param password Пароль для установки
     */
    void setPassword(const QString &password);

private:
    QLineEdit *m_loginEdit;     // Поле ввода логина
    QLineEdit *m_passwordEdit;  // Поле ввода пароля
    
    // Настройка параметров окна
    void setupWindow();
    
    // Создание виджетов интерфейса
    void createWidgets();
    
    // Настройка компоновки виджетов
    void setupLayout();
    
    // Настройка сигнальнов
    void setupConnections();
};
