#include "rainbow_renderer.h"

#include "bmp.h"

#include <cmath>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

constexpr double PI = 3.14159265358979323846;

void RainbowRenderer::setSeed(unsigned int _seed) {
    this->seed = _seed;
}

void RainbowRenderer::setPixelsWide(int _pixels_wide) {
    this->pixels_wide = _pixels_wide;
}

void RainbowRenderer::setPixelsHigh(int _pixels_high) {
    this->pixels_high = _pixels_high;
}

void RainbowRenderer::setNumStartPoints(int _num_start_points) {
    this->num_start_points = _num_start_points;
}

void RainbowRenderer::setColourDepth(int _colour_depth) {
    this->colour_depth = _colour_depth;
}

void RainbowRenderer::setNumIntermediateFrames(int _num_frames) {
    this->num_intermediate_frames = _num_frames;
}

void RainbowRenderer::setDifferenceFunction(float (*_func)(const Colour &, const Colour &)) {
    this->difference_function = _func;
}

void RainbowRenderer::setStartType(StartType _start_type) {
    this->start_type = _start_type;
}

void RainbowRenderer::setFillMode(FillMode _fill_mode) {
    this->fill_mode = _fill_mode;
}

void RainbowRenderer::addColourOrder(ColourOrdering ordering) {
    this->colour_ordering.push_back(ordering);
}

void RainbowRenderer::addStartingHue(int hue) {
    this->startingHues.push_back(hue);
}

void RainbowRenderer::addStartingColour(const Colour &colour) {
    this->startingColours.push_back(colour);
}

void RainbowRenderer::setStripePositions(const std::vector<int> &positions) {
    this->stripePositions = positions;
}

void RainbowRenderer::setMinimumLuminosity(float luminosity) {
    this->minimumLuminosity = luminosity;
}

void RainbowRenderer::setMaximumLuminosity(float luminosity) {
    this->maximumLuminosity = luminosity;
}

void RainbowRenderer::setMinimumSaturation(float saturation) {
    this->minimumSaturation = saturation;
}

void RainbowRenderer::setMaximumSaturation(float saturation) {
    this->maximumSaturation = saturation;
}

/// Initialises starting pixels
void RainbowRenderer::init() {
    this->rng = std::default_random_engine(this->seed);
    this->pixels.resize(this->pixels_wide * this->pixels_high);

    // Compute colours up front: in stripe mode this also reserves per-stripe
    // seed rows in stripeSeeds, which we consume below.
    this->fillColours();

    // Stripe mode short-circuits the normal start_type logic — each stripe
    // gets a full horizontal seed row instead of a handful of start points.
    if (!this->stripePositions.empty()) {
        for (std::size_t i = 0; i < this->stripePositions.size(); ++i) {
            const int y = this->stripePositions[i];
            if (y < 0 || y >= this->pixels_high) {
                std::ostringstream msg;
                msg << "Stripe position " << y << " for stripe " << i
                        << " is outside the image (0.." << (this->pixels_high - 1) << ")";
                throw std::runtime_error(msg.str());
            }
            const std::vector<Colour> &seeds = this->stripeSeeds[i];
            for (int x = 0; x < this->pixels_wide; ++x) {
                Point p(x, y);
                Pixel *pixel = this->getPixelAtPoint(p);
                pixel->colour = seeds[x];
                pixel->is_filled = true;
                this->pushEdge(p);
            }
            std::cout << "Seeded stripe " << i << " at y=" << y
                    << " (" << this->pixels_wide << " pixels)" << std::endl;
        }
        std::cout << "Finished placing stripe seed rows" << std::endl;
        return;
    }

    std::vector<Point> possible_start_points;

    switch (this->start_type) {
        case START_TYPE_CENTRE: {
            possible_start_points.emplace_back(this->pixels_wide / 2, this->pixels_high / 2);
            break;
        }
        case START_TYPE_HORIZONTAL_LINE: {
            int midY = this->pixels_high / 2;
            for (int x = 0; x < this->pixels_wide; ++x) {
                possible_start_points.emplace_back(x, midY);
            }
            break;
        }
        case START_TYPE_VERTICAL_LINE: {
            int midX = this->pixels_wide / 2;
            for (int y = 0; y < this->pixels_high; ++y) {
                possible_start_points.emplace_back(midX, y);
            }
            break;
        }
        case START_TYPE_CORNER: {
            possible_start_points.emplace_back(0, 0);
            possible_start_points.emplace_back(this->pixels_wide - 1, 0);
            possible_start_points.emplace_back(0, this->pixels_high - 1);
            possible_start_points.emplace_back(this->pixels_wide - 1, this->pixels_high - 1);
            break;
        }
        case START_TYPE_EDGE:
        case START_TYPE_RANDOM:
            for (int y = 0; y < this->pixels_high; ++y) {
                for (int x = 0; x < this->pixels_wide; ++x) {
                    if (this->start_type == START_TYPE_RANDOM ||
                        (x == 0 || x == this->pixels_wide - 1 || y == 0 || y == this->pixels_high - 1)) {
                        possible_start_points.emplace_back(x, y);
                    }
                }
            }
            break;
        case START_TYPE_CIRCLE: {
            // Make a circle such that the inner area and outer area are the same (for a square board)
            int radius = int(float(std::min(this->pixels_wide, this->pixels_high)) / sqrt(2 * PI));
            int f = 1 - radius;
            int ddF_x = 0;
            int ddF_y = -2 * radius;
            int x_offset = 0;
            int y_offset = radius;
            int mid_x = this->pixels_wide / 2;
            int mid_y = this->pixels_high / 2;

            possible_start_points.emplace_back(mid_x, mid_y + radius);
            possible_start_points.emplace_back(mid_x, mid_y - radius);
            possible_start_points.emplace_back(mid_x + radius, mid_y);
            possible_start_points.emplace_back(mid_x - radius, mid_y);

            while (x_offset < y_offset) {
                if (f >= 0) {
                    --y_offset;
                    ddF_y += 2;
                    f += ddF_y;
                }
                x_offset++;
                ddF_x += 2;
                f += ddF_x + 1;
                possible_start_points.emplace_back(mid_x + x_offset, mid_y + y_offset);
                possible_start_points.emplace_back(mid_x - x_offset, mid_y + y_offset);
                possible_start_points.emplace_back(mid_x + x_offset, mid_y - y_offset);
                possible_start_points.emplace_back(mid_x - x_offset, mid_y - y_offset);
                possible_start_points.emplace_back(mid_x + y_offset, mid_y + x_offset);
                possible_start_points.emplace_back(mid_x - y_offset, mid_y + x_offset);
                possible_start_points.emplace_back(mid_x + y_offset, mid_y - x_offset);
                possible_start_points.emplace_back(mid_x - y_offset, mid_y - x_offset);
            }
            break;
        }
    }

    if (this->num_start_points.has_value() &&
        static_cast<std::size_t>(*this->num_start_points) > possible_start_points.size()) {
        std::ostringstream msg;
        msg << "Requested " << *this->num_start_points
                << " start points, but only " << possible_start_points.size()
                << " are available for the chosen start type";
        throw std::runtime_error(msg.str());
    }

    std::shuffle(possible_start_points.begin(), possible_start_points.end(), this->rng);
    std::size_t cap = this->num_start_points.value_or(possible_start_points.size());
    for (std::size_t i = 0; i < possible_start_points.size() && i < cap; ++i) {
        std::cout << "Starting in position " << possible_start_points[i] << std::endl;
        fillPoint(possible_start_points[i]);
    }
    std::cout << "Finished placing start points" << std::endl;
}

void RainbowRenderer::fill() {
    switch (this->fill_mode) {
        case FILL_MODE_EDGE:
            this->edge_fill();
            break;
        case FILL_MODE_NEIGHBOUR:
            this->neighbour_fill(false);
            break;
        case FILL_MODE_NEIGHBOUR_AVERAGE:
            this->neighbour_fill(true);
            break;
    }
}

/// Fills remaining spaces with pixels
void RainbowRenderer::edge_fill() {
    const int total_pixels = this->pixels_high * this->pixels_wide;
    const int save_partition = this->num_intermediate_frames > 0
                                   ? total_pixels / this->num_intermediate_frames
                                   : 0;
    const int progress_partition = total_pixels < 100 ? 1 : total_pixels / 100;

    // Per-worker scratch for the parallel min-reduction. Allocated once
    // outside the loop so the scan doesn't reallocate on every placement.
    // Sized to num_workers(); each worker writes only its own slot.
    std::vector<std::size_t> local_best_index(thread_pool_.num_workers(), 0);
    std::vector<float> local_best_diff(thread_pool_.num_workers(),
                                       std::numeric_limits<float>::max());

    while (true) {
        if (this->available_edges.empty() || this->colour_index >= this->colours.size()) {
            std::cout << "Out of edges or colours" << std::endl;
            break;
        }
        const Colour &current_colour = this->colours[this->colour_index];

        // Reset slots to the "no result" sentinel. Workers that get a chunk
        // overwrite their slot; workers that don't (when available_edges is
        // smaller than num_workers()) lose the final reduction comparison.
        std::fill(local_best_diff.begin(), local_best_diff.end(),
                  std::numeric_limits<float>::max());

        thread_pool_.parallel_range(this->available_edges.size(),
                                    [&](std::size_t worker_id, std::size_t start, std::size_t end) {
                                        // Each worker scans its own chunk [start, end) and records
                                        // the local minimum in its dedicated slot. No locking needed
                                        // because slots are per-worker.
                                        std::size_t bi = start;
                                        float bd = this->difference_function(
                                            current_colour,
                                            this->getPixelAtPoint(this->available_edges[start])->colour);
                                        for (std::size_t i = start + 1; i < end; ++i) {
                                            float d = this->difference_function(
                                                current_colour,
                                                this->getPixelAtPoint(this->available_edges[i])->colour);
                                            if (d < bd) {
                                                bd = d;
                                                bi = i;
                                            }
                                        }
                                        local_best_index[worker_id] = bi;
                                        local_best_diff[worker_id] = bd;
                                    });

        // Sequential reduce over per-worker locals. N is tiny (num CPU
        // cores), so this is essentially free.
        std::size_t best_index = local_best_index[0];
        float best_difference = local_best_diff[0];
        for (std::size_t w = 1; w < thread_pool_.num_workers(); ++w) {
            if (local_best_diff[w] < best_difference) {
                best_difference = local_best_diff[w];
                best_index = local_best_index[w];
            }
        }
        Point best_point = this->available_edges[best_index];

        auto neighbours = getNeighboursOfPoint(best_point);
        int neighbour_count = (int) neighbours.size();
        std::shuffle(std::begin(neighbours), std::end(neighbours), this->rng);
        int neighbour_index = 0;
        for (; neighbour_index < neighbour_count; ++neighbour_index) {
            Point &neighbour = neighbours[neighbour_index];
            Pixel *const neighbour_pixel = getPixelAtPoint(neighbour);
            if (neighbour_pixel->is_filled) {
                continue;
            }
            neighbour_pixel->colour = current_colour;
            neighbour_pixel->is_filled = true;
            this->pushEdge(neighbour);
            ++this->colour_index;

            // Incremental cleanup: the only edges whose unfilled-neighbour
            // count just dropped are the filled neighbours of `neighbour`
            // itself. Any that became fully surrounded are popped in O(1).
            // Skip `neighbour` — it was just added by pushEdge above and we
            // don't want to check it against itself.
            for (const Point &m: this->getNeighboursOfPoint(neighbour)) {
                Pixel *m_pixel = this->getPixelAtPoint(m);
                if (m_pixel->edge_index < 0) {
                    continue;
                }
                bool has_open = false;
                for (const Point &mn: this->getNeighboursOfPoint(m)) {
                    if (!this->getPixelAtPoint(mn)->is_filled) {
                        has_open = true;
                        break;
                    }
                }
                if (!has_open) {
                    this->popEdge(static_cast<std::size_t>(m_pixel->edge_index));
                }
            }

            if (this->colour_index % progress_partition == 0) {
                std::cout << "Step " << this->colour_index << " with " << this->available_edges.size() << " edges ("
                        << ((float) this->colour_index / float(total_pixels) * 100)
                        << "%)"
                        << std::endl;
            }
            if (save_partition > 0 && this->colour_index % save_partition == 0) {
                std::ostringstream stream;
                stream << "output_" << int(this->colour_index / save_partition) << ".bmp";
                std::cout << "Saving... " << std::flush;
                this->writeToFile(stream.str());
                std::cout << "Done" << std::endl;
            }
            break;
        }

        if (neighbour_index == neighbour_count) {
            // Best-point was already surrounded when we picked it. Incremental
            // cleanup normally catches this, but starting points placed
            // adjacent to each other in fillPoint can slip through — one may
            // be surrounded before any fill has run.
            this->popEdge(best_index);
        }
    }
}

void RainbowRenderer::neighbour_fill(bool neighbour_average) {
    const int total_pixels = this->pixels_high * this->pixels_wide;
    const int save_partition = this->num_intermediate_frames > 0
                                   ? total_pixels / this->num_intermediate_frames
                                   : 0;
    const int progress_partition = total_pixels < 100 ? 1 : total_pixels / 100;
    // List of places where pixels can be placed (next to neighbours)
    std::vector<Point> availablePoints;
    // Find all neighbours of existing points and add them to the available list
    for (int y = 0; y < this->pixels_high; ++y) {
        for (int x = 0; x < this->pixels_wide; ++x) {
            Point point = Point(x, y);
            Pixel *pixel = this->getPixelAtPoint(point);
            if (!pixel->is_filled) {
                continue;
            }
            for (auto neighbour: this->getNeighboursOfPoint(point)) {
                Pixel *neighbour_pixel = this->getPixelAtPoint(neighbour);
                if (!neighbour_pixel->is_filled && !neighbour_pixel->is_available) {
                    neighbour_pixel->is_available = true;
                    availablePoints.push_back(neighbour);
                }
            }
        }
    }

    // Per-worker scratch for the parallel min-reduction — allocated once
    // outside the placement loop so the scan doesn't reallocate every
    // iteration. Same pattern as edge_fill.
    std::vector<std::size_t> local_best_index(thread_pool_.num_workers(), 0);
    std::vector<float> local_best_diff(thread_pool_.num_workers(),
                                       std::numeric_limits<float>::max());

    // While there are colours to place and available spots to place them
    for (; this->colour_index < this->colours.size() && !availablePoints.empty(); ++this->colour_index) {
        Colour &colour = this->colours[colour_index];

        // Reset the diff slots so workers that don't get a chunk (when
        // availablePoints is smaller than num_workers()) lose the reduce.
        std::fill(local_best_diff.begin(), local_best_diff.end(),
                  std::numeric_limits<float>::max());

        thread_pool_.parallel_range(availablePoints.size(),
                                    [&](std::size_t worker_id, std::size_t start, std::size_t end) {
                                        // Same structure as edge_fill's parallel scan, but the
                                        // per-candidate cost is `getNeighbourDifference` which
                                        // itself walks each candidate's neighbours. Safe to call
                                        // concurrently: it only reads pixel state, and no writes
                                        // to pixels happen until AFTER the reduction below.
                                        std::size_t bi = start;
                                        float bd = this->getNeighbourDifference(
                                            availablePoints[start], colour, neighbour_average);
                                        for (std::size_t i = start + 1; i < end; ++i) {
                                            float d = this->getNeighbourDifference(
                                                availablePoints[i], colour, neighbour_average);
                                            if (d < bd) {
                                                bd = d;
                                                bi = i;
                                            }
                                        }
                                        local_best_index[worker_id] = bi;
                                        local_best_diff[worker_id] = bd;
                                    });

        // Sequential reduction across per-worker locals.
        std::size_t best_index = local_best_index[0];
        float best_difference = local_best_diff[0];
        for (std::size_t w = 1; w < thread_pool_.num_workers(); ++w) {
            if (local_best_diff[w] < best_difference) {
                best_difference = local_best_diff[w];
                best_index = local_best_index[w];
            }
        }
        Point best_point = availablePoints[best_index];

        Pixel *pixel = this->getPixelAtPoint(best_point);
        pixel->is_filled = true;
        pixel->is_available = false;
        pixel->colour = colour;
        availablePoints[best_index] = availablePoints.back();
        availablePoints.pop_back();

        for (auto neighbour: this->getNeighboursOfPoint(best_point)) {
            Pixel *neighbourPixel = this->getPixelAtPoint(neighbour);
            if (!neighbourPixel->is_filled && !neighbourPixel->is_available) {
                neighbourPixel->is_available = true;
                availablePoints.push_back(neighbour);
            }
        }

        if (this->colour_index % progress_partition == 0) {
            std::cout << "Placed " << this->colour_index << "/" << total_pixels << " pixels ("
                    << ((float) this->colour_index / float(total_pixels) * 100) << "%) with "
                    << availablePoints.size()
                    << " spaces available"
                    << std::endl;
        }
        if (save_partition > 0 && this->colour_index % save_partition == 0) {
            std::ostringstream stream;
            stream << "output_" << int(this->colour_index / save_partition) << ".bmp";
            std::cout << "Saving... " << std::flush;
            this->writeToFile(stream.str());
            std::cout << "Done" << std::endl;
        }
    }
}

float RainbowRenderer::getNeighbourDifference(Point point, const Colour &colour, bool neighbour_average) {
    std::vector<float> diffs;
    for (auto neighbour: this->getNeighboursOfPoint(point)) {
        const Pixel *pixel = this->getPixelAtPoint(neighbour);
        if (!pixel->is_filled) {
            continue;
        }
        float diff = this->difference_function(colour, pixel->colour);
        diffs.push_back(diff);
    }
    if (diffs.empty()) {
        return std::numeric_limits<float>::max();
    }

    if (neighbour_average) {
        return (float) (std::accumulate(diffs.begin(), diffs.end(), 0.0) / float(diffs.size()));
    }
    return *std::min_element(diffs.begin(), diffs.end());
}


/// Writes the current content of the pixel board out to file
/// \param _filename
void RainbowRenderer::writeToFile(const std::string &_filename) {
    BMP bmp = BMP(this->pixels_wide, this->pixels_high, false);
    for (int y = 0; y < this->pixels_high; ++y) {
        for (int x = 0; x < this->pixels_wide; ++x) {
            Pixel *pixel = getPixel(x, y);
            bmp.set_pixel(x, y, pixel->colour.b, pixel->colour.g, pixel->colour.r);
        }
    }
    bmp.write(_filename.c_str());
}

Pixel *RainbowRenderer::getPixel(int x, int y) {
    return &this->pixels[y * this->pixels_wide + x];
}

/// Gets the pixel at the given point
/// \param point The point
/// \return The pixel at that point
Pixel *RainbowRenderer::getPixelAtPoint(const Point &point) {
    return &this->pixels[point.y * this->pixels_wide + point.x];
}

/// Gets the neighbour positions of the given point (points over the edge of the board aren ot returned)
/// \param point The point to find the neighbours for
/// \return The number of neighbours
std::vector<Point> RainbowRenderer::getNeighboursOfPoint(const Point &point) const {
    std::vector<Point> neighbours;
    for (int y = -1; y <= 1; ++y) {
        if (y + point.y < 0 || y + point.y >= this->pixels_high) {
            continue;
        }

        for (int x = -1; x <= 1; ++x) {
            if ((x == 0 && y == 0) || x + point.x < 0 || x + point.x >= this->pixels_wide) {
                continue;
            }

            neighbours.emplace_back(x + point.x, y + point.y);
        }
    }
    return neighbours;
}

/// Fills the list of random colours
/// \param colour_depth The number of each unique colours in each channel
void RainbowRenderer::fillColours() {
    // Stripe mode: one target colour per horizontal stripe, one seed row
    // reserved per stripe. Each stripe claims its colours via a shell
    // search around its own target, and later stripes see only what
    // earlier ones didn't claim (so duplicate targets still get distinct
    // colour buckets).
    if (!this->stripePositions.empty()) {
        if (this->stripePositions.size() != this->startingColours.size()) {
            throw std::runtime_error(
                "Number of stripe positions (-P) must match number of starting colours (-C)");
        }
        const std::size_t num_stripes = this->stripePositions.size();
        const std::size_t total_pixels =
                static_cast<std::size_t>(this->pixels_wide) * this->pixels_high;
        const std::size_t pixels_per_stripe = total_pixels / num_stripes;
        if (pixels_per_stripe < static_cast<std::size_t>(this->pixels_wide)) {
            throw std::runtime_error(
                "Each stripe needs at least pixels_wide colours for its seed row");
        }

        // Colours already assigned to some stripe. Prevents a colour matching
        // two targets (e.g. two identical pink stripes) from being handed to
        // both — the later stripe just keeps searching further shells.
        std::set<Colour> claimed;
        this->stripeSeeds.assign(num_stripes, std::vector<Colour>());

        for (std::size_t i = 0; i < num_stripes; ++i) {
            const Colour &target = this->startingColours[i];
            std::vector<Colour> bucket;
            bucket.reserve(pixels_per_stripe);

            int offset = 0;
            std::size_t last_size = 0;
            int dry_passes = 0;
            while (bucket.size() < pixels_per_stripe && dry_passes < 2) {
                for (int r = 0; r <= 255 && bucket.size() < pixels_per_stripe; ++r) {
                    for (int g = 0; g <= 255 && bucket.size() < pixels_per_stripe; ++g) {
                        for (int b = 0; b <= 255 && bucket.size() < pixels_per_stripe; ++b) {
                            Colour candidate(r, g, b);
                            if (claimed.count(candidate)) continue;
                            const auto hsl = rgbToHsl(r, g, b);
                            float sat = std::get<1>(hsl);
                            float lum = std::get<2>(hsl);
                            if (lum < this->minimumLuminosity || lum > this->maximumLuminosity ||
                                sat < this->minimumSaturation || sat > this->maximumSaturation) {
                                continue;
                            }
                            int diff = int(this->difference_function(target, candidate));
                            if (std::abs(diff - offset) <= 1) {
                                bucket.push_back(candidate);
                                claimed.insert(candidate);
                            }
                        }
                    }
                }
                if (bucket.size() == last_size) {
                    ++dry_passes;
                } else {
                    dry_passes = 0;
                    last_size = bucket.size();
                }
                ++offset;
                std::cout << "Stripe " << i << ": " << bucket.size() << "/"
                        << pixels_per_stripe << " colours at offset " << offset << std::endl;
            }

            if (bucket.size() < pixels_per_stripe) {
                std::ostringstream msg;
                msg << "Stripe " << i << " could only find " << bucket.size()
                        << " colours (need " << pixels_per_stripe
                        << "). Widen colour bounds or spread targets further apart.";
                throw std::runtime_error(msg.str());
            }

            // First pixels_wide entries become the seed row for this stripe.
            // Everything else goes to the shared fill pool.
            this->stripeSeeds[i].assign(bucket.begin(), bucket.begin() + this->pixels_wide);
            std::shuffle(this->stripeSeeds[i].begin(), this->stripeSeeds[i].end(), this->rng);
            this->colours.insert(this->colours.end(),
                                 bucket.begin() + this->pixels_wide, bucket.end());
        }

        this->applyColourOrdering(true);
        return;
    }

    if (this->startingHues.empty() && this->startingColours.empty()) {
        if (this->colour_depth == 0) {
            this->colour_depth = ceil(pow(this->pixels_wide * this->pixels_high, 1.0f / 3.0f));
        }

        for (int r = 0; r < this->colour_depth; ++r) {
            for (int g = 0; g < this->colour_depth; ++g) {
                for (int b = 0; b < this->colour_depth; ++b) {
                    this->colours.emplace_back(r * 255 / (this->colour_depth - 1),
                                               g * 255 / (this->colour_depth - 1),
                                               b * 255 / (this->colour_depth - 1));
                }
            }
        }
        std::cout << "Colour depth " << this->colour_depth << " makes " << this->colours.size() << " colours (of "
                << (this->pixels_wide * this->pixels_high) << " pixels)" << std::endl;
    } else {
        std::cout <<
                "Starting hues/colours detected, ignoring colour depth and instead comparing with provided colours."
                << std::endl;
        int offset = 0;
        std::set<Colour> colourSet;
        while (colourSet.size() < this->pixels_wide * this->pixels_high) {
            for (int r = 0; r <= 255; ++r) {
                for (int g = 0; g <= 255; ++g) {
                    for (int b = 0; b <= 255; ++b) {
                        const Colour candidate = Colour(r, g, b);
                        const auto t = rgbToHsl(r, g, b);
                        int hue = int(std::get<0>(t) * 360);
                        float sat = std::get<1>(t);
                        float lum = std::get<2>(t);

                        for (auto targetHue: this->startingHues) {
                            // The hue might be close to the target when wrapping around from 360
                            // so try both and the minimum
                            int hueDifference = std::abs(hue - targetHue);
                            if (hueDifference >= 180) {
                                hueDifference =
                                        targetHue > hue
                                            ? std::abs(hue + 360 - targetHue)
                                            : std::abs(
                                                hue + targetHue - 360);
                            }
                            if (std::abs(hueDifference - offset) <= 1 &&
                                lum >= this->minimumLuminosity && lum <= this->maximumLuminosity &&
                                sat >= this->minimumSaturation && sat <= this->maximumSaturation) {
                                if (colourSet.find(candidate) == colourSet.end()) {
                                    colourSet.insert(candidate);
                                    break;
                                }
                            }
                        }
                        for (const Colour &targetColour: this->startingColours) {
                            int difference = this->difference_function(targetColour, candidate);
                            if (std::abs(difference - offset) <= 1) {
                                if (std::abs(difference - offset) <= 1 &&
                                    lum >= this->minimumLuminosity && lum <= this->maximumLuminosity &&
                                    sat >= this->minimumSaturation && sat <= this->maximumSaturation) {
                                    if (colourSet.find(candidate) == colourSet.end()) {
                                        colourSet.insert(Colour(r, g, b));
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            offset += 1;
            std::cout << "Found " << colourSet.size() << "/" << this->pixels_wide * this->pixels_high <<
                    " colours with offset of " << offset << std::endl;
        }
        this->colours.insert(this->colours.end(), colourSet.begin(), colourSet.end());
    }

    if (this->colours.size() < this->pixels_wide * this->pixels_high) {
        std::ostringstream message;
        message << "All colours were exhausted with only  "
                << 100 * this->colours.size() / (this->pixels_wide * this->pixels_high)
                << "% of the image covered. Please revise input parameters" << std::endl;
        throw std::runtime_error(message.str());
    }

    this->applyColourOrdering(false);
}

void RainbowRenderer::applyColourOrdering(bool default_to_random) {
    if (this->colour_ordering.empty()) {
        if (default_to_random) {
            // Stripe mode wants random so all clusters grow at once.
            this->colour_ordering.push_back({COLOUR_ORDER_RANDOM, false});
        } else {
            this->colour_ordering.push_back({COLOUR_ORDER_LUM, false});
            this->colour_ordering.push_back({COLOUR_ORDER_HUE, false});
            this->colour_ordering.push_back({COLOUR_ORDER_SAT, true});
        }
    }

    for (auto order: this->colour_ordering) {
        bool (*compare_func)(const Colour &c1, const Colour &c2);
        switch (order.ordering_type) {
            case COLOUR_ORDER_RANDOM:
                std::shuffle(std::begin(colours), std::end(colours), rng);
                continue;
            case COLOUR_ORDER_HUE:
                compare_func = compareHue;
                break;
            case COLOUR_ORDER_SAT:
                compare_func = compareSat;
                break;
            case COLOUR_ORDER_LUM:
                compare_func = compareLum;
                break;
            default:
                std::cerr << "Unknown colour ordering " << order.ordering_type << std::endl;
                return;
        }
        if (order.reverse) {
            std::sort(std::rbegin(colours), std::rend(colours), compare_func);
        } else {
            std::sort(std::begin(colours), std::end(colours), compare_func);
        }
    }
}


/// Fills the pixel at the given point
/// \param point The point to place the pixel at
void RainbowRenderer::fillPoint(Point &point) {
    Pixel *pixel = getPixelAtPoint(point);
    pixel->colour = this->colours[this->colour_index];
    pixel->is_filled = true;
    this->pushEdge(point);
    ++this->colour_index;
}

void RainbowRenderer::pushEdge(const Point &p) {
    this->getPixelAtPoint(p)->edge_index = static_cast<int>(this->available_edges.size());
    this->available_edges.push_back(p);
}

void RainbowRenderer::popEdge(std::size_t idx) {
    this->getPixelAtPoint(this->available_edges[idx])->edge_index = -1;
    const std::size_t last = this->available_edges.size() - 1;
    if (idx != last) {
        this->available_edges[idx] = this->available_edges[last];
        // The pixel we just moved into `idx` now lives at `idx`, not `last`.
        this->getPixelAtPoint(this->available_edges[idx])->edge_index = static_cast<int>(idx);
    }
    this->available_edges.pop_back();
}
