#include <pluginlib/class_list_macros.h>
#include <nodelet/nodelet.h>
#include <ros/ros.h>
#include <openni_camera/openni_device_kinect.h>
#include <openni_camera/openni_driver.h>
#include <openni_camera/openni_image.h>
#include <openni_camera/openni_depth_image.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/image_encodings.h>
#include <image_transport/image_transport.h>

using namespace std;
using namespace openni_wrapper;

namespace art_kinect
{
	class KinectPub : public nodelet::Nodelet
	{
		ros::NodeHandle nh;
		image_transport::ImageTransport* it;
		boost::shared_ptr<openni_wrapper::OpenNIDevice> device;
		image_transport::Publisher pub_image, pub_depth;
		sensor_msgs::Image msg_image, msg_depth;
		bool skip_image, skip_depth;
		
		virtual void onInit()
		{
			/// Configure variables.
			nh = getMTNodeHandle();
			it = new image_transport::ImageTransport(nh);
			pub_image = it->advertise("/kinect/image", 10);
			pub_depth = it->advertise("/kinect/depth", 10);
			skip_image = skip_depth = false;
			
			/// Configure image types. Set it as MONO8 type to fool the image_transport plugin.
			msg_image.data.resize(76800); msg_image.width = 320; msg_image.height = 240; msg_image.step = 320;
			msg_depth.data.resize(153600); msg_depth.width = 320; msg_depth.height = 480; msg_depth.step = 320;
			msg_image.encoding = sensor_msgs::image_encodings::MONO8;
			msg_depth.encoding = sensor_msgs::image_encodings::MONO8;
			
			/// Load and configure the Kinect device.
			OpenNIDriver& driver = OpenNIDriver::getInstance();
			driver.updateDeviceList();
			device = driver.getDeviceByIndex(0);
			XnMapOutputMode output_mode;
			output_mode.nXRes = XN_VGA_X_RES;
			output_mode.nYRes = XN_VGA_Y_RES;
			output_mode.nFPS = 30;
			device->setDepthOutputMode(output_mode);
			device->setImageOutputMode(output_mode);
			device->setDepthRegistration(true);
			dynamic_cast<DeviceKinect*>(device.get())->setDebayeringMethod(ImageBayerGRBG::EdgeAwareWeighted);

			/// Register callbacks and start the streams.
			device->registerImageCallback(&KinectPub::image_callback, *this);
			device->registerDepthCallback(&KinectPub::depth_callback, *this);
			device->startImageStream();
			device->startDepthStream();
		}

		~KinectPub()
		{
			delete it;
		}

		void image_callback(boost::shared_ptr<Image> image, void* cookie)
		{
			if(!skip_image)
			{
				msg_image.header.stamp = ros::Time::now();
				image->fillGrayscale(320, 240, &msg_image.data[0], 320);
				pub_image.publish(msg_image);
			}
			skip_image = !skip_image;
		}
		
		void depth_callback(boost::shared_ptr<DepthImage> image, void* cookie)
		{
			if(!skip_depth)
			{
				msg_depth.header.stamp = ros::Time::now();
				image->fillDepthImageRaw(320, 240, (short*) &msg_depth.data[0], 640);
				pub_depth.publish(msg_depth);
			}
			skip_depth = !skip_depth;
		}
	};
}

PLUGINLIB_DECLARE_CLASS(art_kinect, kinect_pub, art_kinect::KinectPub, nodelet::Nodelet);
