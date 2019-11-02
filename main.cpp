#include <iostream>
#include <cstdlib>
#include <list>
#include <iostream>
#include <algorithm>
#include <random>
#include <ctime>

#include "bmp.h"

auto rng = std::default_random_engine(std::random_device{}());


int PIXELS_WIDE = 512;
int PIXELS_HIGH = 512;
int COLOUR_DEPTH = 256;
const int COLOUR_COUNT = COLOUR_DEPTH * COLOUR_DEPTH * COLOUR_DEPTH;


int getHue(int red, int green, int blue) {

	float fmin = std::min(std::min(red, green), blue);
	float fmax = std::max(std::max(red, green), blue);

	if (fmin == fmax) {
		return 0;
	}

	float hue = 0;
	if (fmax == red) {
		hue = (green - blue) / (fmax - fmin);

	}
	else if (fmax == green) {
		hue = 2 + (blue - red) / (fmax - fmin);

	}
	else {
		hue = 4 + (red - green) / (fmax - fmin);
	}

	hue = hue * 60;
	if (hue < 0) hue = hue + 360;
	return int(hue);
}


struct Colour {
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t hue = 0;
	bool filled = false;

	Colour() {}

	Colour(uint8_t _r, uint8_t _g, uint8_t _b) {
		this->r = _r;
		this->g = _g;
		this->b = _b;
		this->hue = getHue(_r, _g, _b);
		this->filled = false;
	}
};

std::ostream &operator<<(std::ostream &os, const Colour &colour) {
	return os << (int)colour.r << "," << (int)colour.g << "," << (int)colour.b;
}

struct Point {
	int x;
	int y;

	Point() {
		x = -1;
		y = -1;
	}

	Point(int _x, int _y) {
		this->x = _x;
		this->y = _y;
	}

	bool operator==(const Point &other) {
		return other.x == this->x && other.y == this->y;
	}


};

std::ostream &operator<<(std::ostream &os, const Point &point) {
	return os << point.x << "," << point.y;
}

int ColourDiff(Colour* colour_1, Colour* colour_2) {
	//int d = colour_1->hue - colour_2->hue;
	//std::cout << d << std::endl;
	//return  abs(d);

	int r = (int)colour_1->r - (int)colour_2->r;
	int g = (int)colour_1->g - (int)colour_2->g;
	int b = (int)colour_1->b - (int)colour_2->b;
	return r * r + g * g + b * b;
}

Colour* GetPixel(std::vector<Colour>& pixels, int x, int y) {
	return &pixels[y  * PIXELS_WIDE + x];
}

Colour* GetPixelAtPoint(std::vector<Colour>& pixels, Point &point) {
	//std::cout << "getting " << point << " at " << (point.y * PIXELS_HIGH + point.x) << std::endl;
	return &pixels[point.y * PIXELS_WIDE + point.x];
}

int FillPointNeighbours(std::vector<Point>& neighbours, Point point) {
	neighbours.clear();
	for (int y = -1; y <= 1; ++y) {
		if (y + point.y < 0 || y + point.y >= PIXELS_HIGH) {
			continue;
		}

		for (int x = -1; x <= 1; ++x) {
			if ((x == 0 && y == 0) || x + point.x < 0 || x + point.x >= PIXELS_WIDE) {
				continue;
			}

			neighbours.push_back(Point(x + point.x, y + point.y));
			//neighbours[neighbour_count].x = x + point.x;
			//neighbours[neighbour_count].y = y + point.y;
			//++neighbour_count;
		}
	}
	//    std::cout << "Neighbour count " << neighbour_count << std::endl;
	return neighbours.size();// neighbour_count;
}

void ShuffleColours(Colour *const array, size_t n) {
	if (n > 1) {
		size_t i;
		for (i = 0; i < n - 1; i++) {
			size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
			Colour t = array[j];
			array[j] = array[i];
			array[i] = t;
		}
	}
}

int main(int argc, char *argv[]) {
	time_t start_time = time(0);
	std::vector<Colour> colours(COLOUR_COUNT);
	for (int r = 0; r < COLOUR_DEPTH; ++r) {
		for (int g = 0; g < COLOUR_DEPTH; ++g) {
			for (int b = 0; b < COLOUR_DEPTH; ++b) {
				Colour* colour = &colours[r * COLOUR_DEPTH * COLOUR_DEPTH + g * COLOUR_DEPTH + b];
				colour->r = r * 255 / (COLOUR_DEPTH - 1);
				colour->g = g * 255 / (COLOUR_DEPTH - 1);
				colour->b = b * 255 / (COLOUR_DEPTH - 1);
				colour->hue = getHue(colour->r, colour->g, colour->b);
			}
		}
	}
	std::shuffle(std::begin(colours), std::end(colours), rng);
	std::vector<Colour> pixels(PIXELS_WIDE * PIXELS_HIGH);
	std::cout << "Pixels " << pixels.size() << std::endl;
	std::list<Point> available_edges;
	std::vector<Point> neighbours(8);
	int colour_index = 0;

	Point centre_point = Point(PIXELS_WIDE / 2, PIXELS_HIGH / 2);
	available_edges.push_back(centre_point);
	Colour* centre_pixel = GetPixelAtPoint(pixels, centre_point);
	centre_pixel->r = colours[colour_index].r;
	centre_pixel->g = colours[colour_index].g;
	centre_pixel->b = colours[colour_index].b;
	centre_pixel->filled = true;
	
	/*
	for (int i = 0; i < 100; ++i) {
		int rand_x = rng() % PIXELS_WIDE;
		int rand_y = rng() % PIXELS_HIGH;

		Point random_point = Point(rand_x, rand_y);
		available_edges.push_back(random_point);
		Colour* centre_pixel = GetPixelAtPoint(pixels, random_point);
		centre_pixel->r = colours[colour_index].r;
		centre_pixel->g = colours[colour_index].g;
		centre_pixel->b = colours[colour_index].b;
		centre_pixel->hue = colours[colour_index].hue;
		centre_pixel->filled = true;
		colour_index += 1;
	}
	int i = 0;*/
	/*for (long y = 0; y < PIXELS_HIGH; ++y) {
		for (long x = 0; x < PIXELS_WIDE; ++x) {
			if (x == 0 || y == 0 || x == PIXELS_WIDE - 1 || y == PIXELS_HIGH - 1) 
			//if ((x == 0 || x == PIXELS_WIDE - 1  ) && (y == 0 || y == PIXELS_HIGH - 1))
			//if (((x == 0 || x == PIXELS_WIDE - 1) && y == PIXELS_HIGH / 2) || ((y == 0 || y == PIXELS_HIGH - 1) && x == PIXELS_WIDE / 2))
			{
				//++i;
				//if (i % 64 == 0)
				{
					Colour* colour = GetPixel(pixels, x, y);
					colour->r = colours[colour_index].r;
					colour->g = colours[colour_index].g;
					colour->b = colours[colour_index].b;
					++colour_index;
					colour->filled = true;
					available_edges.push_back(Point(x, y));
				}
			}

		}
	}*/

	while (true) {
		if (available_edges.empty() || colour_index == COLOUR_COUNT) {
			std::cout << "Out of edges or colours" << std::endl;
			break;
		}
		Colour* current_colour = &colours[colour_index];
		//std::cout << "Current colour " << current_colour << std::endl;
		//std::cout << "Available edges " << available_edges.size() << std::endl;
		Point best_point;
		int best_difference = INT32_MAX;
		for (auto &available_edge : available_edges) {
			int diff = ColourDiff(current_colour, GetPixelAtPoint(pixels, available_edge));
			if (diff < best_difference) {
				best_point = available_edge;
				best_difference = diff;
			}
		}
		//std::cout << "Best point " << best_point << " at " << best_difference << std::endl;

		int neighbour_count = FillPointNeighbours(neighbours, best_point);
		std::shuffle(std::begin(neighbours), std::end(neighbours), rng);
		bool placed_point = false;
		for (int i = 0; i < neighbour_count; ++i) {
			Point &neighbour = neighbours[i];
			//std::cout << "Neighbour " << i << " " << neighbour << std::endl;
			Colour* neighbour_colour = GetPixelAtPoint(pixels, neighbour);
			if (neighbour_colour->filled) {
				continue;
			}

			neighbour_colour->r = current_colour->r;
			neighbour_colour->g = current_colour->g;
			neighbour_colour->b = current_colour->b;
			neighbour_colour->hue = current_colour->hue;
			neighbour_colour->filled = true;
			available_edges.push_back(neighbour);
			colour_index += 1;
			placed_point = true;
			//std::cout << "PLACED" << std::endl;

			if (colour_index % 1000 == 0) {
				std::cout << colour_index << ": " << available_edges.size() << " (" << ((float)colour_index / (PIXELS_HIGH * PIXELS_WIDE) * 100) << "%)" << std::endl;
			}
			break;
		}

		if (!placed_point) {
			//std::cout << "Point is exhausted of neighbours " << best_point << std::endl;
			available_edges.remove(best_point);
		}
	}
	time_t end_time = time(0);
	std::cout << "Completed in " << (end_time - start_time) << "s" << std::endl;

	BMP bmp = BMP(PIXELS_WIDE, PIXELS_HIGH, false);
	for (long y = 0; y < PIXELS_HIGH; ++y) {
		for (long x = 0; x < PIXELS_WIDE; ++x) {

			Colour* colour = GetPixel(pixels, x, y);
			bmp.set_pixel(x, y, colour->b, colour->g, colour->r);
			//            bmp.data.push_back(colour.r);
			//            bmp.data.push_back(colour.g);
			//            bmp.data.push_back(colour.b);
			//            std::cout << "Putting colour " << colour << std::endl;
		}
	}
	bmp.write("output.bmp");

	//    delete[] pixels;
	//    delete[] colours;
//	std::cout << "???" << std::endl;
	return 0;
}