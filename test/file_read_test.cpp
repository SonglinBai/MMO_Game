
#include <iostream>
#include <fstream>

int main() {
    std::ifstream file("resources/map/map_demo.txt");

    std::string ret;
    int x, y = 0;

    std::string line;
    while(std::getline(file, line)) {
        ret.append(line);
        y++;
    }
    x = line.length();

    std::cout << "size: (" << x << "," << y << ")" << std::endl << ret << std::endl;


}
