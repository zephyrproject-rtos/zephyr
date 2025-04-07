# Adapted from OpenCV tutorial
# https://docs.opencv.org/3.4/dc/dbb/tutorial_py_calibration.html

import os
import sys

import cv2 as cv
import numpy as np

if len(sys.argv) != 2:
    print('usage: ' + sys.argv[0] + ' path/to/file.jpg')
    sys.exit(1)
basename = os.path.splitext(sys.argv[1])[0]

# Read the input image
img = cv.imread(sys.argv[1])
gray = cv.cvtColor(img, cv.COLOR_BGR2GRAY)

# Prepare points matching the checkerboard shape, like (0,0,0), (1,0,0), (2,0,0) ..., (6,5,0)
objp = np.zeros((6 * 7, 3), np.float32)
objp[:, :2] = np.mgrid[0:7, 0:6].T.reshape(-1, 2)

# Arrays to store object points and image points from all the images.
objpoints = []  # 3d point in real world space
imgpoints = []  # 2d points in image plane.

ret, corners = cv.findChessboardCorners(gray, (7, 6), None)
if not ret:
    raise ValueError('Chessboard corners could not be found')

# If found, add object points, image points (after refining them)
objpoints.append(objp)
criteria = (cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_MAX_ITER, 30, 0.001)
corners2 = cv.cornerSubPix(gray, corners, (11, 11), (-1, -1), criteria)
imgpoints.append(corners2)

# Draw and display the corners
cv.drawChessboardCorners(img, (7, 6), corners2, ret)
cv.imwrite(basename + '.annotated.jpg', img)

# Get the calibration values using the difference between the object points and image points
ret, matrix, dist, _, _ = cv.calibrateCamera(objpoints, imgpoints, gray.shape[::-1], None, None)

# Save the calibration data to files
np.savetxt(basename + '.matrix.txt', matrix)
np.savetxt(basename + '.dist.txt', dist)
print(f'Calibration data saved as {basename}.*.txt')
