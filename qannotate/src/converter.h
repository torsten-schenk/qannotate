#pragma once

#include <QWidget>
#include <QLineEdit>

#include "ui_converter.h"

class Converter : public QWidget {
	Q_OBJECT
	public:
		Converter(QWidget *parent = nullptr);

	public slots:
		void encode();
		void decode();

	private:
		Ui::Form _ui;
		QLineEdit *_line1;
		QLineEdit *_line2;
};