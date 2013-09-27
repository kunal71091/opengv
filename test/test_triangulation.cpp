/******************************************************************************
 * Author:   Laurent Kneip                                                    *
 * Contact:  kneip.laurent@gmail.com                                          *
 * License:  Copyright (c) 2013 Laurent Kneip, ANU. All rights reserved.      *
 *                                                                            *
 * Redistribution and use in source and binary forms, with or without         *
 * modification, are permitted provided that the following conditions         *
 * are met:                                                                   *
 * * Redistributions of source code must retain the above copyright           *
 *   notice, this list of conditions and the following disclaimer.            *
 * * Redistributions in binary form must reproduce the above copyright        *
 *   notice, this list of conditions and the following disclaimer in the      *
 *   documentation and/or other materials provided with the distribution.     *
 * * Neither the name of ANU nor the names of its contributors may be         *
 *   used to endorse or promote products derived from this software without   *
 *   specific prior written permission.                                       *
 *                                                                            *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"*
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  *
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE *
 * ARE DISCLAIMED. IN NO EVENT SHALL ANU OR THE CONTRIBUTORS BE LIABLE        *
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL *
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR *
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER *
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT         *
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY  *
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF     *
 * SUCH DAMAGE.                                                               *
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <opengv/triangulation/methods.hpp>
#include <opengv/relative_pose/CentralRelativeAdapter.hpp>
#include <sstream>
#include <fstream>

#include "random_generators.hpp"
#include "experiment_helpers.hpp"
#include "time_measurement.hpp"


using namespace std;
using namespace Eigen;
using namespace opengv;

int main( int argc, char** argv )
{
  // initialize random seed
  initializeRandomSeed();

  //set experiment parameters
  double noise = 0.5;
  double outlierFraction = 0.0;
  size_t numberPoints = 10;

  //generate a random pose for viewpoint 1
  translation_t position1 = Eigen::Vector3d::Zero();
  rotation_t rotation1 = Eigen::Matrix3d::Identity();

  //generate a random pose for viewpoint 2
  translation_t position2 = generateRandomTranslation(2.0);
  rotation_t rotation2 = generateRandomRotation(0.5);

  //create a fake central camera
  translations_t camOffsets;
  rotations_t camRotations;
  generateCentralCameraSystem( camOffsets, camRotations );

  //derive correspondences based on random point-cloud
  bearingVectors_t bearingVectors1;
  bearingVectors_t bearingVectors2;
  std::vector<int> matches;
  std::vector<int> camCorrespondences1; //unused in the central case
  std::vector<int> camCorrespondences2; //unused in the central case
  Eigen::MatrixXd gt(3,numberPoints);
  generateRandom2D2DCorrespondences(
      position1, rotation1, position2, rotation2,
      camOffsets, camRotations, numberPoints, noise, outlierFraction,
      bearingVectors1, bearingVectors2, matches,
      camCorrespondences1, camCorrespondences2, gt );
    
    
  std::cout << "the original points are: " << std::endl << gt;
  std::cout << std::endl << std::endl;

  //Extract the relative pose
  translation_t position; rotation_t rotation;
  extractRelativePose(
      position1, position2, rotation1, rotation2, position, rotation, false );

  //print experiment characteristics
  printExperimentCharacteristics( position, rotation, noise, outlierFraction );

  //create a central relative adapter and pass the relative pose
  relative_pose::CentralRelativeAdapter adapter(
      bearingVectors1,
      bearingVectors2,
      matches,
      position,
      rotation);

  //timer
  struct timeval tic;
  struct timeval toc;
  size_t iterations = 100;

  //run experiments
  std::cout << "running triangulation algorithm 1" << std::endl << std::endl;
  double triangulate_time;
  MatrixXd triangulate_results(3,numberPoints);
  for(size_t j = 0; j < numberPoints; j++)
  {
    gettimeofday( &tic, 0 );
    for(size_t i = 0; i < iterations; i++)
      triangulate_results.block<3,1>(0,j) =
          triangulation::triangulate(adapter,j);
    gettimeofday( &toc, 0 );
    triangulate_time = TIMETODOUBLE(timeval_minus(toc,tic)) / iterations;
  }
  std::cout << "timing: " << triangulate_time << std::endl;
  std::cout << "triangulation result: " << std::endl;
  std::cout << triangulate_results << std::endl;
  MatrixXd error(1,numberPoints);
  for(size_t i = 0; i < numberPoints; i++)
  {
    Vector3d singleError = triangulate_results.col(i) - gt.col(i);
    error(0,i) = singleError.norm();
  }
  std::cout << "triangulation error is: " << std::endl;
  std::cout << error << std::endl << std::endl;

  std::cout << "running triangulation algorithm 2" << std::endl << std::endl;
  double triangulate2_time;
  MatrixXd triangulate2_results(3,numberPoints);
  for(size_t j = 0; j < numberPoints; j++)
  {
    gettimeofday( &tic, 0 );
    for(size_t i = 0; i < iterations; i++)
      triangulate2_results.block<3,1>(0,j) =
          triangulation::triangulate2(adapter,j);
    gettimeofday( &toc, 0 );
    triangulate2_time = TIMETODOUBLE(timeval_minus(toc,tic)) / iterations;
  }
  std::cout << "timing: " << triangulate2_time << std::endl;
  std::cout << "triangulation result: " << std::endl;
  std::cout << triangulate2_results << std::endl;
  for(size_t i = 0; i < numberPoints; i++)
  {
    Vector3d singleError = triangulate2_results.col(i) - gt.col(i);
    error(0,i) = singleError.norm();
  }
  std::cout << "triangulation error is: " << std::endl << error << std::endl;
}