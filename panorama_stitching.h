#pragma once

#include <QtWidgets/QMainWindow>
#include <QList>
#include <QFiledialog>
#include "ui_panorama_stitching.h"
#include "Image.h" // for image set


class panorama_stitching : public QMainWindow
{
	Q_OBJECT

public:
	panorama_stitching(QWidget *parent = Q_NULLPTR);

private:
	Ui::panorama_stitchingClass ui;

	// List for image set
	QList<Image*> list_img;
	// Iterator for list_img. Point current image set for dispaly
	QList<Image*>::iterator cur;

	// Focal length and scale factor
	int factor;

	// Update widgets in main window
	void update_window();

	// Cylindrical projection in disregard of Y axis transform
	void cylindricalProjection_v1(QList<Image*>::iterator cur);
	// Normal Cylindrical projection
	void cylindricalProjection_v2(QList<Image*>::iterator cur);
	
private slots:
	void openImage();
	void scrol();
	void cylindricalProjection_v1_caller();
	void cylindricalProjection_v2_caller();
	void prev_img();
	void next_img();
};
