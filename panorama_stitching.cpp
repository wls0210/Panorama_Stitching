#include "panorama_stitching.h"

#define TESTx

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
	connect(ui.pushButton_save, SIGNAL(clicked()), this, SLOT(save_img()));
	connect(ui.pushButton_prev, SIGNAL(clicked()), this, SLOT(prev_img()));
	connect(ui.pushButton_next, SIGNAL(clicked()), this, SLOT(next_img()));
	connect(ui.actionKeypoint_detection, SIGNAL(triggered()), this, SLOT(keypoint_detection_caller()));
}

// Generate Gaussian filter tab
void genFilterTab(double filter[7], double sigma2) {
	double sum = 0;
	for (int64_t i = -3; i < 4; i++) {
		filter[i + 3] = exp(-1 * i * i / (2 * sigma2)) / (sqrt(2 * 3.1415926535897932) * sigma2);
		sum += filter[i + 3];
	}
	sum = 1 / sum;
	for (int i = 0; i < 7; i++) {
		filter[i] *= sum;
	}
}


// Keypoint detecting fuction
void panorama_stitching::keypoint_detection(QList<Image*>::iterator cur) {
	
	if ((*cur)->cylindrical == NULL) return;

	// DoG fields
	double* DoG[4];

	// input image
	QImage* img_in = (*cur)->cylindrical;

	int width = img_in->width();
	int height = img_in->height();

	// Gray scale image
	double* gray_ori = new double[width * height];

	// Convert to grayscale (YCbCr)
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			double y;
			int r = img_in->pixelColor(i, j).red();
			int g = img_in->pixelColor(i, j).green();
			int b = img_in->pixelColor(i, j).blue();
			y = 0.299 * r + 0.587 * g + 0.114 * b;
			gray_ori[width * j + i] = y;
		}
	}

	// filtered images
	double* img_1DG_A;
	double* img_2DG_A;
	double* img_1DG_B;
	double* img_2DG_B;
	
	// copy gray scale image
	double* gray = new double[width * height];
	memcpy(gray, gray_ori, sizeof(double) * width * height);

	// for 3 scale image
	for (int space = 0; space < 3; space++) {

		// image class for test
#ifdef TEST
		QImage qwer(width, height, QImage::Format_RGB888);
#endif
		// filter tabs
		double filter[7] = { 0 };
		// filter parameter
		double sigma2 = pow(1.41, space);

		// filtering parameter
		int tab_size = 7;
		int tab_half = tab_size / 2;

		// generate filter
		genFilterTab(filter, sigma2);


		// row filtered image "A"
		img_1DG_A = new double[width * height];
		// row filtering
		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				double s = 0;
				for (int k = 0; k < tab_size; k++) {
					if (i - tab_half + k < 0 || i - tab_half + k >= width) continue;
					s += gray[j * width + (i - tab_half + k)] * filter[k];
				}
				img_1DG_A[j * width + i] = s;
			}
		}

		// column filtered image "A"
		img_2DG_A = new double[width * height];
		// column filtering
		for (int j = 0; j < width; j++) {
			for (int i = 0; i < height; i++) {
				double s = 0;
				for (int k = 0; k < tab_size; k++) {
					if (i - tab_half + k < 0 || i - tab_half + k >= height) continue;
					s += img_1DG_A[(i - tab_half + k) * width + j] * filter[k];
				}
				img_2DG_A[i * width + j] = s;
#ifdef TEST
				QColor c(s, s, s);
				qwer.setPixelColor(j, i, c);
#endif
			}
		}
		delete[](img_1DG_A);

#ifdef TEST
		qwer.save("test_G_"+QString::number(space)+"_0.jpg", "JPG");
#endif

		// Scale up for 4 times
		for (int s = 0; s < 4; s++) {
			// update blur parametor
			sigma2 *= 2;
			// update filter tab
			genFilterTab(filter, sigma2);

			// row filtered image "B"
			img_1DG_B = new double[width * height];
			// row filtering
			for (int j = 0; j < height; j++) {
				for (int i = 0; i < width; i++) {
					double s = 0;
					for (int k = 0; k < tab_size; k++) {
						if (i - tab_half + k < 0 || i - tab_half + k >= width) continue;
						s += gray[j * width + (i - tab_half + k)] * filter[k];
					}
					img_1DG_B[j * width + i] = s;
				}
			}

			// column filtered image "A"
			img_2DG_B = new double[width * height];
			// column filtering
			for (int j = 0; j < width; j++) {
				for (int i = 0; i < height; i++) {
					double s = 0;
					for (int k = 0; k < tab_size; k++) {
						if (i - tab_half + k < 0 || i - tab_half + k >= height) continue;
						s += img_1DG_B[(i - tab_half + k) * width + j] * filter[k];
					}
					img_2DG_B[i * width + j] = s;
#ifdef TEST
					QColor c(s, s, s);
					qwer.setPixelColor(j, i, c);
#endif
				}
			}
			delete[](img_1DG_B);

#ifdef TEST
			qwer.save("test_G_" + QString::number(space) + "_" + QString::number(s + 1) + ".jpg", "JPG");
#endif

			// Allocate DoG field
			DoG[s] = new double[width * height];
			// Compute DoG
			for (int i = 0; i < height; i++) {
				for (int j = 0; j < width; j++) {
					DoG[s][i * width + j] = img_2DG_B[i * width + j] - img_2DG_A[i * width + j];
#ifdef TEST
					double q = DoG[s][i * width + j] * 32 + 128;
					QColor c(q, q, q);
					qwer.setPixelColor(j, i, c);
#endif
				}
			}
#ifdef TEST
			qwer.save("test_DoG_" + QString::number(space) + "_" + QString::number(s + 1) + ".jpg", "JPG");
#endif

			delete[](img_2DG_A);

			// move to next blur scale
			img_2DG_A = img_2DG_B;
		}

		delete[](img_2DG_B);

		// detect keypoint
		// for 2 DoG field
		for (int i = 1; i < 3; i++) {
			for (int j = 1; j < height - 1; j++) {
				for (int k = 1; k < width - 1; k++) {
					double cent = DoG[i][j * width + k];
					// check adjacent pixels
					if (abs(cent) > 0.03 &&
						(cent < DoG[i - 1][(j - 1) * width + (k - 1)] && cent < DoG[i - 1][(j - 1) * width + (k)] &&
							cent < DoG[i - 1][(j - 1) * width + (k + 1)] && cent < DoG[i - 1][(j)*width + (k - 1)] &&
							cent < DoG[i - 1][(j)*width + (k + 1)] && cent < DoG[i - 1][(j + 1) * width + (k - 1)] &&
							cent < DoG[i - 1][(j + 1) * width + (k)] && cent < DoG[i - 1][(j + 1) * width + (k + 1)] &&
							cent < DoG[i + 1][(j - 1) * width + (k - 1)] && cent < DoG[i + 1][(j - 1) * width + (k)] &&
							cent < DoG[i + 1][(j - 1) * width + (k + 1)] && cent < DoG[i + 1][(j)*width + (k - 1)] &&
							cent < DoG[i + 1][(j)*width + (k + 1)] && cent < DoG[i + 1][(j + 1) * width + (k - 1)] &&
							cent < DoG[i + 1][(j + 1) * width + (k)] && cent < DoG[i + 1][(j + 1) * width + (k + 1)] &&
							cent < DoG[i - 1][(j)*width + (k)] && cent < DoG[i + 1][(j)*width + (k)]) ||
						(cent > DoG[i - 1][(j - 1) * width + (k - 1)] && cent > DoG[i - 1][(j - 1) * width + (k)] &&
							cent > DoG[i - 1][(j - 1) * width + (k + 1)] && cent > DoG[i - 1][(j)*width + (k - 1)] &&
							cent > DoG[i - 1][(j)*width + (k + 1)] && cent > DoG[i - 1][(j + 1) * width + (k - 1)] &&
							cent > DoG[i - 1][(j + 1) * width + (k)] && cent > DoG[i - 1][(j + 1) * width + (k + 1)] &&
							cent > DoG[i + 1][(j - 1) * width + (k - 1)] && cent > DoG[i + 1][(j - 1) * width + (k)] &&
							cent > DoG[i + 1][(j - 1) * width + (k + 1)] && cent > DoG[i + 1][(j)*width + (k - 1)] &&
							cent > DoG[i + 1][(j)*width + (k + 1)] && cent > DoG[i + 1][(j + 1) * width + (k - 1)] &&
							cent > DoG[i + 1][(j + 1) * width + (k)] && cent > DoG[i + 1][(j + 1) * width + (k + 1)] &&
							cent > DoG[i - 1][(j)*width + (k)] && cent > DoG[i + 1][(j)*width + (k)])) {

						// check extreme point by Hessian matrix
						double dxx = gray[(j + 1) * width + k] + gray[(j - 1) * width + k] - 2 * gray[j * width + k];
						double dyy = gray[j * width + (k + 1)] + gray[j * width + (k - 1)] - 2 * gray[j * width + k];
						double dxy = (gray[(j + 1) * width + (k + 1)] - gray[(j - 1) * width + (k + 1)]
							- gray[(j + 1) * width + (k - 1)] + gray[(j - 1) * width + (k - 1)]) / 4; 

						double tr = dxx + dyy;
						double det = dxx * dyy - dxy * dxy;

						// Mark out extreme point
						if (tr * tr * 10 < 121 * det && det > 0) {
							QPainter p((*cur)->cylindrical);
							p.fillRect(k * pow(2, space) - 1, j * pow(2, space) - 1, 3, 3, Qt::red);
							p.end();
						}

					}
				}
			}
		}
		// Gaussian filter for anti aliasing
		genFilterTab(filter, 2.56);
		// Anti aliasing
		double* img_1DG_tmp = new double[width * height];
		for (int j = 0; j < height; j++) {
			for (int i = 0; i < width; i++) {
				double s = 0;
				for (int k = 0; k < tab_size; k++) {
					if (i - tab_half + k < 0 || i - tab_half + k >= width) continue;
					s += gray[j * width + (i - tab_half + k)] * filter[k];
				}
				img_1DG_tmp[j * width + i] = s;
			}
		}
		double* img_2DG_tmp = new double[width * height];
		for (int j = 0; j < width; j++) {
			for (int i = 0; i < height; i++) {
				double s = 0;
				for (int k = 0; k < tab_size; k++) {
					if (i - tab_half + k < 0 || i - tab_half + k >= height) continue;
					s += img_1DG_tmp[(i - tab_half + k) * width + j] * filter[k];
				}
				img_2DG_tmp[i * width + j] = s;
			}
		}

		delete[](img_1DG_tmp);

		// Down sampling
		width /= 2;
		height /= 2;
		double* _gray = new double[width * height];
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				_gray[i * width + j] = img_2DG_tmp[i * 4 * width + j * 2];
			}
		}
		delete[](img_2DG_tmp);
		delete[](gray);

		gray = _gray;
	}

	// Mem free
	for (int i = 0; i < 4; i++)
		delete[](DoG[i]);
	delete[](gray_ori);

#ifdef TEST
	(*cur)->cylindrical->save("result.bmp", "BMP");
#endif

	return;
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
		double _sqrt = sqrt((x  - width / 2.0) * (x - width / 2.0) + factor* factor);

		// for Y axis
		for (int j = 0; j < height; j++) {
			// transform Y
			double y = (height / -2.0 + j) / factor * _sqrt + (height / 2.0);

			// do anything for out of range pixel
			if (y < 0 || y >= height - 1) continue;

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

void panorama_stitching::save_img() {
	(*cur)->cylindrical->save("save.bmp", "BMP");
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


void panorama_stitching::keypoint_detection_caller() {
	// Project for each image list
	QList<Image*>::iterator it = list_img.begin();
	for (; it != list_img.end(); it++) {
		keypoint_detection(it);
	}

	update_window();
}
