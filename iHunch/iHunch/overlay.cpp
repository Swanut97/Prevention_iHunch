#include "overlay.h"
#include <QLabel>

overlay::overlay(QWidget *parent)
	: QWidget(parent), ui(new Ui::overlay)
{
	ui->setupUi(this);

	this->setGeometry(QRect(0, 0, 100, 50));
	this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

//	overlayColorChange(1);
}

overlay::~overlay()
{
	delete ui;
}

void overlay::overlayColorChange(int mode)
{
	QLabel* label = ui->label;
	
	if (mode == 0) {	//������ ���� ������
		label->setText(QString::fromLocal8Bit("����"));
//		label->setStyleSheet("QLabel { background-color : red; color : black; }");
	}
	else if (mode == 1) { // �������� �ʷϺ�
		label->setText(QString::fromLocal8Bit("����"));
//		label->setStyleSheet("QLabel { background-color : lime; color : black; }");
	}
}
