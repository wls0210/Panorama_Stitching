#include "panorama_stitching.h"

panorama_stitching::panorama_stitching(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	// init forcal point ans scale factor
	factor = 800;

	// Signal connect
	connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(openImage()));
	connect(ui.actionDisable_Y_axis, SIGNAL(triggered()), this, SLOT(cylindricalProjection_v1_caller()));
	connect(ui.actionAble_Y_axis, SIGNAL(triggered()), this, SLOT(cylindricalProjection_v2_caller()));
	connect(ui.horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(scrol()));
	connect(ui.pushButton_prev, SIGNAL(clicked()), this, SLOT(prev_img()));
	connect(ui.pushButton_next, SIGNAL(clicked()), this, SLOT(next_img()));
}

// Cylindrical projection in disregard of Y axis transform
void panorama_stitching::cylindricalProjection_v1(QList<Image*>::iterator cur) {
	// check input for projection
	if (list_img.isEmpty()) return;

	// check size of image
	int width = (*cur)->input->width();
	int height = (*cur)->input->height();

	// delete previous projected image
	delete((*cur)->cylindrical);

	// allocate memory for new projected image and init
	(*cur)->cylindrical = new QImage(width, height, QImage::Format_RGB888);
	(*cur)->cylindrical->fill(Qt::black);

	// for X axis
	for (int i = 0; i < width; i++) {
		// normalize x 
		double norm = (width / -2.0 + i) / factor;
		// do anything for out of range pixel
		if (norm < -1.5707963267948966 || norm > 1.5707963267948966) continue;
		// transform X
		double x = factor * tan(norm) + (width / 2.0);
		// do anything for out of range pixel
		if (x < 0 || x >= width) continue;

		// compute relative coordinate
		double dx = x - (int)x;

		// for Y axis
		for (int j = 0; j < height; j++) {
			// load 2 pixel around target
			QColor left((*cur)->input->pixelColor((int)x, j));
			QColor right((*cur)->input->pixelColor((int)x + 1, j));

			// interpolation
			QColor col(((right.red() - left.red()) * dx + left.red()), ((right.green() - left.green()) * dx + left.green()), ((right.blue() - left.blue()) * dx + left.blue()));

			// set Pixcel
			(*cur)->cylindrical->setPixelColor(i, j, col);
		}
	}
}
// Normal Cylindrical projection
void panorama_stitching::cylindricalProjection_v2(QList<Image*>::iterator cur) {
	// check input for projection
	if (list_img.isEmpty()) return;

	// check size of image
	int width = (*cur)->input->width();
	int height = (*cur)->input->height();

	// delete previous projected image
	delete((*cur)->cylindrical);

	// allocate memory for new projected image and init
	(*cur)->cylindrical = new QImage(width, height, QImage::Format_RGB888);
	(*cur)->cylindrical->fill(Qt::black);

	// for X axis
	for (int i = 0; i < width; i++) {
		// normalize x 
		double norm = (width / -2.0 + i) / factor;
		// do anything for out of range pixel
		if (norm < -1.5707963267948966 || norm > 1.5707963267948966) continue;
		// transform X
		double x = factor * tan(norm) + (width / 2.0);
		// do anything for out of range pixel
		if (x < 0 || x >= width) continue;

		// compute relative coordinate
		double dx = x - (int)x;

		// for transform Y
		double _sqrt = sqrt(norm * norm + 1) * factor;

		// for Y axis
		for (int j = 0; j < height; j++) {
			// transform Y
			double y = (height / -2.0 + j) / factor * _sqrt + (height / 2.0);

			// do anything for out of range pixel
			if (y < 0 || y >= height) continue;

			// load 4 pixel around target
			QColor p00((*cur)->input->pixelColor((int)x, (int)y));
			QColor p01((*cur)->input->pixelColor((int)x, (int)y + 1));
			QColor p10((*cur)->input->pixelColor((int)x + 1, (int)y));
			QColor p11((*cur)->input->pixelColor((int)x + 1, (int)y + 1));

			// compute relative coordinates to target
			y -= (int)y;

			// interpolation
			QColor p0(((p10.red() - p00.red()) * dx + p00.red()), ((p10.green() - p00.green()) * dx + p00.green()), ((p10.blue() - p00.blue()) * dx + p00.blue()));
			QColor p1(((p11.red() - p01.red()) * dx + p01.red()), ((p11.green() - p01.green()) * dx + p01.green()), ((p11.blue() - p01.blue()) * dx + p01.blue()));
			QColor p(((p1.red() - p0.red()) * y + p0.red()), ((p1.green() - p0.green()) * y + p0.green()), ((p1.blue() - p0.blue()) * y + p0.blue()));

			// set Pixcel
			(*cur)->cylindrical->setPixelColor(i, j, p);
		}
	}
}

// Update widgets in main window
void panorama_stitching::update_window() {
	// if image list is empty, there is nothing to do
	if (list_img.isEmpty()) return;

	// move button setting
	if (cur == list_img.begin()) ui.pushButton_prev->setEnabled(false);
	else ui.pushButton_prev->setEnabled(true);
	if (cur + 1 == list_img.end()) ui.pushButton_next->setEnabled(false);
	else ui.pushButton_next->setEnabled(true);

	// Display images
	ui.label->setPixmap(QPixmap::fromImage((*cur)->input->scaled(ui.label->width(), ui.label->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
	if ((*cur)->cylindrical == NULL) {
		QImage* temp= new QImage(ui.label_2->width(), ui.label_2->height(), QImage::Format_RGB888);
		temp->fill(Qt::black);
		ui.label_2->setPixmap(QPixmap::fromImage(*temp));
		delete(temp);
	}
	else {
		ui.label_2->setPixmap(QPixmap::fromImage((*cur)->cylindrical->scaled(ui.label_2->width(), ui.label_2->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
	}

	// Display image path
	ui.label_img_name->setText(*(*cur)->name);

}

// Open images with dialog
void panorama_stitching::openImage() {
	// get file path with qt dialog api
	QStringList* files = new QStringList(QFileDialog::getOpenFileNames(this, tr("Select image file"), NULL, tr("")));

	// check file exsist
	if (files->isEmpty()) return;

	// for each file path
	foreach(const QString& file, *files) {

		// allocate new image class to load image file
		QImage* new_img = new QImage();

		// load image and check availablity
		if (!new_img->load(file)) return;

		// allocate image set
		Image* img = new Image();

		// setting image set
		img->input = new_img;
		img->name = new QString(file);

		// push to list
		list_img.append(img);

		// move cursor
		cur = list_img.end() - 1;
	}

	update_window();
}

// Move slider
void panorama_stitching::scrol() {
	// update factor label
	factor = ui.horizontalSlider->value();
	ui.label_3->setText(QString::number(factor));

}

// Push move button
void panorama_stitching::prev_img() {
	if (cur != list_img.begin() && !list_img.isEmpty()) {
		cur -= 1;
		update_window();
	}
}
void panorama_stitching::next_img() {
	if (cur + 1 != list_img.end() && !list_img.isEmpty()) {
		cur += 1;
		update_window();
	}
}

// Click menu bar for projection
void panorama_stitching::cylindricalProjection_v1_caller() {
	// Project for each image list
	QList<Image*>::iterator it = list_img.begin();
	for (; it != list_img.end(); it++) {
		cylindricalProjection_v1(it);
	}

	update_window();
}
void panorama_stitching::cylindricalProjection_v2_caller() {
	// Project for each image list
	QList<Image*>::iterator it = list_img.begin();
	for (; it != list_img.end(); it++) {
		cylindricalProjection_v2(it);
	}

	update_window();
}