#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class Dialog; }
QT_END_NAMESPACE


class Dialog : public QDialog
{
    Q_OBJECT

public:
    int data_2 = 100;
    Dialog(QWidget *parent = nullptr);
    ~Dialog();



private slots:
    void on_pushButton_clicked();


    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_6_clicked();


private:
    Ui::Dialog *ui;
    int data;
};
#endif // DIALOG_H
