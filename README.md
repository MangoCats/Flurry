# Flurry
Quick code to read an rstp:// stream and check a region for "bright pixels" corresponding to ice on a condenser coil

Environment setup:

sudo apt install build-essential cmake xcb libopencv-dev libpaho-mqtt-dev

To build:

mkdir build
cd build
cmake ..
make

or, run the ./build.sh script.

Executable is in build folder named: Flurry


Notes:

https://stackoverflow.com/questions/11541154/checking-images-for-similarity->

// Compare two images by getting the L2 error (square-root of sum of squared>
double getSimilarity( const Mat A, const Mat B ) {
    if ( A.rows > 0 && A.rows == B.rows && A.cols > 0 && A.cols == B.cols ) {
        // Calculate the L2 relative error between images.
        double errorL2 = norm( A, B, NORM_L2 );
        // Convert to a reasonable scale, since L2 error is summed across al>
        double similarity = errorL2 / (double)( A.rows * A.cols );
        return similarity;
    }
    else {
        //Images have a different size
        return 100000000.0;  // Return a bad value
    }
}

