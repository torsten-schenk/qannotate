#ifdef _WIN32
#include <QtPlugin>
	Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
	Q_IMPORT_PLUGIN(QSvgIconPlugin)
#endif

#include "mainwindow.h"

static MainWindow *mainWindow;

int main(int argn, char **argv)
{
#ifdef _WIN32
	if (AttachConsole(ATTACH_PARENT_PROCESS)) {
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	}
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
#endif
	QApplication app(argn, argv);

	mainWindow = new MainWindow();
	mainWindow->show();

	return app.exec();
}
