#include <iostream>
#include <cmath>
#include <array>

using namespace std;

typedef array<double, 6> Pos; // [x, y, z, yaw, pitch, roll]

Pos kinematics(double l1, double l2, double alpha, double beta, double gamma, double phi){
    Pos pos;
    pos[0] = (l1 * sin(beta * M_PI / 180) + l2 * sin((gamma - beta) * M_PI / 180)) * cos(alpha * M_PI / 180);
    pos[1] = (l1 * sin(beta * M_PI / 180) + l2 * sin((gamma - beta) * M_PI / 180)) * sin(alpha * M_PI / 180);
    pos[2] = l1 * cos(beta * M_PI / 180) - l2 * cos((gamma - beta) * M_PI / 180);
    pos[3] = alpha;
    pos[4] = gamma - beta - 90;
    pos[5] = phi;
    return pos;
}

int main() {
    double l1 = 130; //longueur premier bras
    double l2 = 100; //longueur second bras
    double alpha = 81; //angle pivot_1 deg
    double beta = 19; //angle pivot_2 deg
    double gamma = 141; //angle pivot_3 deg
    double phi = 60; //angle pivot_4 deg

    Pos pos = kinematics(l1, l2, alpha, beta, gamma, phi);
    cout << "Position: (" << pos[0] << ", " << pos[1] << ", " << pos[2] << ")" << endl;
    cout << "Orientation: (yaw: " << pos[3] << ", pitch: " << pos[4] << ", roll: " << pos[5] << ")" << endl;
    return 0;
}


