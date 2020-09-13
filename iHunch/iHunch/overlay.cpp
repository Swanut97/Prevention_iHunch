#include "overlay.h"

overlay::overlay(QWidget *parent)
	: QWidget(parent), ui(new Ui::overlay)
{
	ui->setupUi(this);

	this->setGeometry(QRect(0, 0, 30, 30));
	this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

	overlayColorChange(1);
}

overlay::~overlay()
{
	delete ui;
}

void overlay::overlayColorChange(int mode)
{
	if (mode == 0) {	//������ ���� ������
		this->setStyleSheet("background-color: red");
	}
	else if (mode == 1) { // �������� �ʷϺ�
		this->setStyleSheet("background-color: lime");
	}
}
