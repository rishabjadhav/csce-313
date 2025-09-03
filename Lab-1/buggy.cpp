#include <iostream>
#include <cstring>

struct Point {
    int x, y;

    Point () : x(), y() {}
    Point (int _x, int _y) : x(_x), y(_y) {}
};

class Shape {
    int vertices;
    Point** points;

    public:
        Shape (int _vertices) {
            vertices = _vertices;
            points = new Point*[vertices+1];
        }

    ~Shape () {
        // remember to delete dynamically allocated variables in the destructor
        delete[] points;
    }

    void addPoints (Point** pts) {
        for (int i = 0; i < vertices; i++) {
            // copy over memory from each element in pts into points
            memcpy(&points[i], &pts[i%vertices], sizeof(Point));
        }
    }

    // corrected return type of area() function
    double area () {
        int temp = 0;
        for (int i = 0; i < vertices; i++) {
            // use % vertices to wrap around to first element and avoid invalid access
            int lhs = (*points[i]).x * (*points[(i+1)%vertices]).y;
            int rhs = points[(i+1)%vertices]->x * points[i]->y;
            temp += (lhs - rhs);
        }
        double area = abs(temp)/2.0;
        return area;
    }
};

int main () {
    // create the following points using the three different methods
    //        of defining structs:
    //          tri1 = (0, 0)
    //          tri2 = (1, 2)
    //          tri3 = (2, 0)

    Point _tri1 = {0, 0};
    Point _tri2; _tri2.x = 1; _tri2.y = 2;
    Point _tri3{2, 0};

    Point* tri1 = &_tri1;
    Point* tri2 = &_tri2;
    Point* tri3 = &_tri3;

    // adding points to tri
    Point* triPts[3] = {tri1, tri2, tri3};
    Shape* tri = new Shape(3);
    tri->addPoints(triPts);

    // adding points to quad
    Point* quad1 = new Point(0, 0);
    Point* quad2 = new Point(0, 2);
    Point* quad3 = new Point(2, 2);
    Point* quad4 = new Point(2, 0);

    Point* quadPts[4] = {quad1, quad2, quad3, quad4};
    Shape* quad = new Shape(4);
    quad->addPoints(quadPts);

    std::cout << tri->area() << std::endl;
    std::cout << quad->area() << std::endl;

    // delete all dynamically allocated objects
    delete tri;

    delete quad1;
    delete quad2;
    delete quad3;
    delete quad4;
    delete quad;
}
