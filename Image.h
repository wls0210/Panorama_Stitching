#pragma once

#include <QImage>

// Store date of image
class Image
{
public:
	Image() {
		input = NULL;
		cylindrical = NULL;
		name = NULL;
	}
	~Image() {
		delete(input);
		delete(cylindrical);
		delete(name);
	}

	QImage* input;			// Input Image
	QImage* cylindrical;	// Projected Image
	QString* name;			// Path of image
};

