#include <vtkDataSetWriter.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkResampleToImage.h>
#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridReader.h>

#include <mgard/compress.hpp>

int main(int argc, char *argv[]) {
  // Parse command line arguments
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " Filename" << std::endl;
    return EXIT_FAILURE;
  }

  std::string vtkFile = argv[1];

  // load the vtk file
  vtkSmartPointer<vtkUnstructuredGridReader> reader =
      vtkSmartPointer<vtkUnstructuredGridReader>::New();
  reader->SetFileName(vtkFile.c_str());
  reader->Update();

  // get the specific polydata and check the results
  vtkUnstructuredGrid *unsGridData = reader->GetOutput();

  // unsGridData->Print(std::cout);
  // get field array direactly and put it into the data set
  vtkPointData *pointData = unsGridData->GetPointData();

  auto pointDataArray = pointData->GetScalars("v_center_dist");

  // pointDataArray->Print(std::cout);

  // set the pointDataArray into the mgard
  long unsigned int N = pointDataArray->GetNumberOfTuples();
  const mgard::TensorMeshHierarchy<1, double> hierarchy({N});

  double *const u = (double *)(pointDataArray->GetVoidPointer(0));

  // Now we set the compression parameters. First we select the norm in which to
  // control the compression error. We choose from the family of supported norms
  // by setting a smoothness parameter `s`. `s = 0` corresponds to the `L²`
  // norm.
  const double s = 0;
  // Next we set the absolute error tolerance `τ`. The approximating dataset `ũ`
  // generated by MGARD will satisfy `‖u - ũ‖_{L²} ≤ τ`.
  const double tolerance = 0.000001;

  const mgard::CompressedDataset<1, double> compressed =
      mgard::compress(hierarchy, u, s, tolerance);
  std::cout << "compressed ok" << std::endl;

  // `compressed` contains the compressed data buffer. We can query its size in
  // bytes with the `size` member function.
  std::cout << "compression ratio for raw data set is: "
            << static_cast<double>(N * sizeof(*u)) / compressed.size()
            << std::endl;

  // try to create a sample based on
  // refer to this
  // https://gitlab.kitware.com/vtk/vtk/-/blob/master/Filters/Core/Testing/Cxx/TestResampleToImage.cxx
  vtkNew<vtkResampleToImage> resample;
  resample->SetUseInputBounds(true);
  // 2d case, the value at the last dim is 1
  resample->SetSamplingDimensions(50, 50, 1);
  // resample->SetInputConnection(reader->GetOutputPort());
  resample->SetInputDataObject(unsGridData);
  resample->Update();

  vtkImageData *resampledImage = resample->GetOutput();
  // resampledImage->Print(std::cout);

  // output resampled data for double checking

  vtkSmartPointer<vtkDataSetWriter> writer =
      vtkSmartPointer<vtkDataSetWriter>::New();
  std::string fileSuffix = vtkFile.substr(0, vtkFile.size() - 4);
  std::string outputFileName = fileSuffix + std::string("Resample.vtk");

  writer->SetFileName(outputFileName.c_str());

  // get the specific polydata and check the results
  writer->SetInputData(resampledImage);
  // Optional - set the mode. The default is binary.
  // writer->SetDataModeToBinary();
  // writer->SetDataModeToAscii();
  writer->Write();

  // put into the mgard to check

  auto reamplePointDataArray =
      resampledImage->GetPointData()->GetScalars("v_center_dist");

  // pointDataArray->Print(std::cout);

  // set the pointDataArray into the mgard
  long unsigned int rN = reamplePointDataArray->GetNumberOfTuples();
  const mgard::TensorMeshHierarchy<1, double> rhierarchy({N});

  double *const ru = (double *)(reamplePointDataArray->GetVoidPointer(0));

  const mgard::CompressedDataset<1, double> rcompressed =
      mgard::compress(rhierarchy, ru, s, tolerance);
  std::cout << "compressed sampled data ok" << std::endl;

  // `compressed` contains the compressed data buffer. We can query its size in
  // bytes with the `size` member function.
  std::cout << "compression ratio for sampled data set is: "
            << static_cast<double>(rN * sizeof(*ru)) / rcompressed.size()
            << std::endl;
}