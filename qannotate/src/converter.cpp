#include <QWidget>
#include <QPushButton>
#include <QLayout>
#include <QLineEdit>

#include "types.h"
#include "converter.h"

Converter::Converter(QWidget *parent)
	:	QWidget(parent)
{
	_ui.setupUi(this);
	this->setWindowFlags(this->windowFlags() | Qt::Window);
	this->setWindowIcon(QIcon(":/images/logo_icon.svg"));
	this->setWindowTitle("QAnnotate - Converter");

	connect(_ui.pushButton, SIGNAL(clicked()), this, SLOT(encode()));
	connect(_ui.pushButton_2, SIGNAL(clicked()), this, SLOT(decode()));
}

void Converter::encode()
{
	_ui.lineEdit_2->setText(curspec::DirFrontend::encodeFilename(_ui.lineEdit->text()));
}

void Converter::decode()
{
	_ui.lineEdit->setText(curspec::DirFrontend::decodeFilename(_ui.lineEdit_2->text()));
}