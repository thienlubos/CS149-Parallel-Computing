// wb.h

#ifndef MANDELBROTUTILS_H
#define MANDELBROTUTILS_H

#include <iostream>
#include <fstream>


//@@ Thien Lu added
void writeSpeedResults(const std::string& filename, const float index, const float value) {
    // Open the file in append mode
    std::ofstream outfile(filename, std::ios_base::app);
    
    if (outfile.is_open()) { // Check if the file is open
        outfile << index << " " << value << std::endl; // Write a value to the file
        outfile.close(); // Close the file
        std::cout << "Value written to file successfully." << std::endl;
    } else {
        std::cerr << "Unable to open file " << filename << std::endl;
    }
}

#endif // MANDELBROTUTILS_H
