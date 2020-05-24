#include "panorama_stitching.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	panorama_stitching w;
	w.show();
	return a.exec();
}
