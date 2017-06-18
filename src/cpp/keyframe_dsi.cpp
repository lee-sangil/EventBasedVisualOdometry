#include "keyframe_dsi.h"

namespace emvs{

KeyframeDSI::KeyframeDSI(double im_height, double im_width, double min_depth, double max_depth,
						 int N_planes, double fx, double fy)
	: im_height_(im_height), im_width_(im_width),
	  min_depth_(min_depth), max_depth_(max_depth),
	  N_planes_(N_planes), fx_(fx), fy_(fy)
{
	// Reserve space for data
	planes_scaling_.resize(N_planes_);
	planes_depths_.resize(N_planes_);
	dsi_.resize(N_planes_);

	// Find uniformly spaced planes, either in depth or inverse depth
	// *Original EMVS used depth - "we used depth instead of inverse depth in the
	// DSI since it provided better results in scenes with finite depth variations"
	// *EVO used inverse depth - "allow for mapping far away objects, we discretize
	// the DSI volume using depth planes uniformly spaced in inverse depth"
	// *EMVS used N=100, EVO used N=50

	// Uniform depth
	planes_depths_ = linspace(min_depth_, max_depth_, N_planes_);

	// Uniform inverse depth
	// std::vector<double> planes_depths = linspace(1/min_depth_, 1/max_depth_, N_planes_);
	// std::for_each(planes_depths.begin(), planes_depths.end(), [](double& d){ d = 1/d;});

	// Find scaling to real world units
	double cone_angle_x = atan((im_width/2) / fx_); //half of FOV
	double cone_angle_y = atan((im_height/2) / fy_); //half of FOV

	for(int i=0; i<N_planes_; i++)
	{
		double depth = planes_depths_[i];
		double frustumHeight = 2 * depth * tan(cone_angle_y);
		double frustumWidth = 2 * depth * tan(cone_angle_x);

		//"pixels" to meters (i.e. d_pixels * scale = d_world)
		double scale_x = frustumHeight / im_height;
		double scale_y = frustumWidth / im_width;
		planes_scaling_[i] = {scale_x, scale_y};

		dsi_[i] = cv::Mat::zeros(im_height_, im_width_, EVENT_IMAGE_TYPE);
	}

	resetDSI();
}

void KeyframeDSI::resetDSI()
{
	// Set zeros
	for (int i=0; i<N_planes_; i++)
		dsi_[i].setTo(0);
}

// Returns filtered set of 3D points from DSI, in keyframe frame.
PointCloud KeyframeDSI::getFiltered3dPoints()
{
	//getDepthmap: gaussian blur on each layer, then take max from each layer to return wxh depth map (opencv)
	cv::Mat depthmap = cv::Mat(im_height_, im_width_, EVENT_IMAGE_TYPE, cv::Scalar(0));
	getDepthmap(depthmap); // TODO move getDepthMap to inside kf_dsi
	std::cout << "depthmap in emvsnode: " << cv::countNonZero(depthmap) << std::endl;
	// cv::Mat1b idx = depthmap > 0;
	// depthmap.setTo(255, idx);
	// cv::imshow(OPENCV_WINDOW, depthmap);
	// cv::waitKey(1);

	//ProjectDsiPointsTo3d: get 3d point coordinates from filtered depth map (manually?) TODO
	PointCloud new_points;
	projectDepthmapTo3d(new_points);

	//RadiusFilter: radius outlier removal of resulting (use pcl) TODO heck this
	// radiusFilter(new_points);
	return new_points;
}


void KeyframeDSI::getDepthmap(cv::Mat& output)
{
	int gauss_filter_size = 5;
	int median_filter_size = 15; //window size must be odd

	// TODO Gaussian filter should be ran on confidence map, after findMaxVals3d
	// 1. Gaussian filter on each layer
	// std::cout << "showing gaussian filtered dsi." << std::endl;
	cv::Mat filtered(im_height_, im_width_, EVENT_IMAGE_TYPE, cv::Scalar(0));
	for(int i=0; i<N_planes_; i++)
	{
		cv::GaussianBlur(dsi_[i], filtered, cv::Size(gauss_filter_size, gauss_filter_size), 3);
		filtered.copyTo(dsi_[i]);
	}

	// 2. Find max across all of the images, and their location
	//TODO figure out better way to do this
	cv::Mat max_locs = cv::Mat(im_height_, im_width_, EVENT_IMAGE_TYPE, cv::Scalar(0));
	cv::Mat max_vals = cv::Mat(im_height_, im_width_, EVENT_IMAGE_TYPE, cv::Scalar(0));
	findMaxVals3D(dsi_, output);

	// TODO 3. threshold on max to get intermediate depthmap


	// cv::Mat interm_depth_map = cv::Mat(im_height_, im_width_, EVENT_IMAGE_TYPE, cv::Scalar(0));
	// interm_depth_map = confidence_map > (imfilter(confidence_map, filter) - C); // C=-2

	// 4. Median filter on depth map
	// cv::Mat final_depthmap(im_height_, im_width_, EVENT_IMAGE_TYPE, cv::Scalar(0));
	// std::cout << "Image channels: " << max_locs.channels() << " " << filtered.channels() << std::endl;
	// cv::medianBlur(max_locs, filtered, median_filter_size);
	// TODO matlab code finds median over non-zero values - may have to implement custom

}

void KeyframeDSI::projectDepthmapTo3d(PointCloud& output)
{

}

} // end namespace emvs
