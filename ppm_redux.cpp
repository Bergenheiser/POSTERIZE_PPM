#include <vector>
#include <cmath>
#include <set>
#include <iostream>
#include <fstream>
#include <stdexcept>

using std::cout;
using std::ifstream;
using std::ofstream;
using std::pair;
using std::runtime_error;
using std::set;
using std::sqrt;
using std::vector;

using namespace std;
class Color
{
    double value;

public:
    double get_value() const { return value; }
    void set_value(double v) { this->value = v; }
    Color(double v) { this->value = v; }
    Color() = default;
    ~Color() = default;
};

class Pixel
{
    Color r, g, b;

public:
    int index;
    Pixel(Color red, Color green, Color blue)
    {
        this->r = red;
        this->g = green;
        this->b = blue;
    }
    Pixel() = default;
    ~Pixel() = default;
    Pixel(int r, int g, int b) : Pixel(Color(r), Color(g), Color(b)) {}

    double get_r() const { return r.get_value(); }
    void set_r(Color red) { this->r = red; }
    void set_r(int red) { this->r = Color(red); }

    double get_g() const { return g.get_value(); }
    void set_g(Color green) { this->g = green; }
    void set_g(double green) { this->g = Color(green); }

    double get_b() const { return b.get_value(); }
    void set_b(Color blue) { this->b = blue; }
    void set_b(double blue) { this->b = Color(blue); }

    double distance(const Pixel &p2);
    bool operator==(const Pixel &other) const;
    bool operator!=(const Pixel &b) const;
};

double Pixel::distance(const Pixel &p2)
{
    int redDiff = this->get_r() - p2.get_r();
    int greenDiff = this->get_g() - p2.get_g();
    int blueDiff = this->get_b() - p2.get_b();

    return std::sqrt(std::pow(redDiff, 2) + std::pow(greenDiff, 2) + std::pow(blueDiff, 2));
}

bool Pixel::operator==(const Pixel &b) const
{
    int tolerance = 1; // Tolérance pour les arrondis
    return abs(this->get_r() - b.get_r()) <= tolerance &&
           abs(this->get_g() - b.get_g()) <= tolerance &&
           abs(this->get_b() - b.get_b()) <= tolerance;
}
bool Pixel::operator!=(const Pixel &b) const
{
    return !(*this == b);
}
class Cluster
{
    Pixel barycentre;
    vector<Pixel> Pixels;

public:
    Cluster() = default;
    Cluster(const vector<Pixel> &pixels) : Pixels(pixels) {}
    ~Cluster() = default;

    void set_barycentre(Pixel b) { this->barycentre = b; }
    Pixel get_barycentre() const { return barycentre; }

    vector<Pixel> &getPixels() { return this->Pixels; }
    void clearPixels() { this->Pixels.clear(); }
    void addPixel(const Pixel e) { this->Pixels.emplace_back(e); }
    void setPixels(const vector<Pixel> &pixels) { Pixels = pixels; }
};

class Image
{
    int width;
    int height;
    vector<Pixel> data;
    vector<Pixel> k_redux;

public:
    Image(int w, int h) : width(w > 0 ? w : 0), height(h > 0 ? h : 0), data(height * width) {}
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    Image() = default;
    Image(string in);
    Pixel &getPixel(int index);
    void kmeans(int k);
    void write_redux(string out);
};

Pixel &Image::getPixel(int index)
{
    if (index < this->data.size())
    {
        return data[index];
    }
    else
    {
        throw std::runtime_error("outofbounds exception");
    }
}

Image::Image(string filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Error reading file");
    }

    string magicNumber, comment;
    int width, height, maxValue;
    file >> magicNumber >> comment;

    if (magicNumber != "P3")
    {
        throw std::runtime_error("File is not of P3 magic number");
    }

    file >> width >> height >> maxValue;
    this->width = width;
    this->height = height;
    this->data.resize(width * height);

    for (int i = 0; i < width * height; i++)
    {
        int r, g, b;
        if (!(file >> r >> g >> b))
        {
            throw std::runtime_error("Error reading pixel data");
        }
        this->getPixel(i).set_r(Color(r));
        this->getPixel(i).set_g(Color(g));
        this->getPixel(i).set_b(Color(b));
        this->getPixel(i).index = i;
    }

    file.close();

    if (this->data.size() == 0)
    {
        throw std::runtime_error("Empty pixel data set from input stream.");
    }
}

Cluster &getNearestCluster(Pixel &p, vector<Cluster> &c)
{
    int closestIndex = 0;
    double minDistance = p.distance(c[0].get_barycentre());
    for (int i = 1; i < c.size(); i++)
    {
        double currentDistance = p.distance(c[i].get_barycentre());
        if (currentDistance < minDistance)
        {
            minDistance = currentDistance;
            closestIndex = i;
        }
    }
    return c[closestIndex];
}

Pixel getNearestBarycenter(Pixel &p, vector<Pixel> k_redux)
{
    double minDistance = std::numeric_limits<double>::max();
    Pixel nearestBarycenter = k_redux.at(0);
    for (const auto &Bary : k_redux)
    {
        double currentDistance = p.distance(Bary);
        if (currentDistance < minDistance)
        {
            minDistance = currentDistance;
            nearestBarycenter = Bary;
        }
    }
    return nearestBarycenter;
}
void compute_barycentre(Cluster &c)
{

    if (!c.getPixels().empty())
    {
        vector<Pixel> &Pixels = c.getPixels();
        double red = 0.0, green = 0.0, blue = 0.0;
        for (auto i = Pixels.begin(); i != Pixels.end(); ++i)
        {
            red += i->get_r();
            green += i->get_g();
            blue += i->get_b();
        }
        int v_size = Pixels.size();

        int avg_red = static_cast<int>(red / v_size);
        int avg_green = static_cast<int>(green / v_size);
        int avg_blue = static_cast<int>(blue / v_size);
        c.set_barycentre(Pixel(avg_red, avg_green, avg_blue));
    }
    else
    {
        throw std::runtime_error("empty pixel set on compute barycentre");
    }
}

/**
 * Applies the k-means clustering algorithm to the image data.
 *
 * @param k The number of clusters to create.
 * @throws std::runtime_error if the image data is not found (empty data set).
 */
void Image::kmeans(int k)
{
    if (this->data.size() > 0)
    {
        vector<Pixel> output(this->data.size());
        vector<Cluster> clusters;
        for (int i = 0; i < k; ++i)
        {
            Cluster c;
            // Distance approximativement égale entre les index des premiers barycentres.
            int temp_index = i * this->data.size() / k;
            c.set_barycentre(this->data[temp_index]);
            cout << "Cluster index: " << i << " - Barycentre(indexOfPixel): " << c.get_barycentre().index << endl;
            clusters.emplace_back(c);
        }

        bool changement = true;
        while (changement)
        {
            changement = false;
            for (Pixel &i : this->data)
            {
                Cluster &closest = getNearestCluster(i, clusters);
                closest.addPixel(i);
            }

            // Update barycenter
            for (Cluster &curr : clusters)
            {
                if (!curr.getPixels().empty())
                {
                    Pixel temp = curr.get_barycentre();
                    compute_barycentre(curr);
                    if (!(temp == curr.get_barycentre()))
                    {
                        changement = true;
                        curr.clearPixels();
                    }
                }
            }
        }
        for (Cluster &redux : clusters)
        {
            this->k_redux.emplace_back(redux.get_barycentre());
        }
    }
    else
    {
        throw std::runtime_error("image data not found (empty data set)");
    }
}

void Image::write_redux(string out)
{
    cout << "Output Name: " << out << endl;
    cout << "Size W*H: " << this->width << " " << this->height << endl;

    if (this->k_redux.size() > 0)
    {

        ofstream file(out);
        if (!file)
        {
            cout << "Error opening file." << endl;
            return;
        }

        file << "P3" << endl;
        file << width << " " << height << endl;
        file << "255" << endl;

        // Write the pixel data to the file
        for (int index = 0; index < this->getHeight() * this->getWidth(); ++index)
        {
            Pixel &current = data[index];
            Pixel destinationPixel = getNearestBarycenter(current, k_redux);
            file << destinationPixel.get_r() << " " << destinationPixel.get_g() << " " << destinationPixel.get_b();
            // Insert newline if index reaches the width of the image
            if ((index + 1) % this->getWidth() == 0)
            {
                file << endl;
            }
            else
            {
                file << " ";
            }
        }
        file << endl;
        file.close();
        cout << "Export success!" << endl;
    }

    else
    {
        throw std::runtime_error("redux data not found (empty redux set)");
    }
}

int main()
{
    //////CONFIG////////
    string directory = "./assets/";
    string fileName = "plage";
    int redux_factor = 2;

    ///////FORMAT///////
    string input = directory + fileName + ".ppm";
    string output = fileName + "_K" + to_string(redux_factor) + "_OUTPUT.ppm";

    ////////EXEC////////
    Image img(input);
    img.kmeans(redux_factor);
    img.write_redux(output);
    return 0;
}
