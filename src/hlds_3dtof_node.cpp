/*******************************************************************************
* Copyright (c) 2018, Hitachi-LG Data Storage, Inc.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
* * Neither the name of the copyright holder nor the names of its
*   contributors may be used to endorse or promote products derived from
*   this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

 /* Authors: Jeehoon Yang, Wayne Rust */
 /* maintainer: Jeehoon Yang */

#include <ros/ros.h>
#include <sensor_msgs/image_encodings.h>
#include <pcl_ros/point_cloud.h>
#include "hldstof/tof.h"

using namespace std;
using namespace hlds;

int main(int argc, char* argv[])
{
  // Initialize ROS
  ros::init (argc, argv, "hlds_3dtof");
  std::string frame_id;
  ros::NodeHandle nh;
  sensor_msgs::PointCloud2 cloud;
  sensor_msgs::ImagePtr msgDepth;
  sensor_msgs::ImagePtr msgDepthRaw;
  sensor_msgs::ImagePtr msgIr;
  std::string file_name;
  std::string cloud_topic;
  ros::Publisher cloudPublisher;
  ros::Publisher depthPublisher;
  ros::Publisher depthrawPublisher;
  ros::Publisher irPublisher;

  // Initialize HLDS 3D TOF
  Result ret = Result::OK;
  int numoftof = 0;
  TofManager tm;
  const TofInfo* ptofinfo = nullptr;
  FrameDepth frame; //Create instances for reading frames
  Frame3d frame3d;  //Create instances for 3D data after conversion
  FrameIr frameir;  //Create instances for reading IR frames

  // Open TOF Manager (Read tof.ini file)
  nh.getParam("ini_path", tm.inifilepath);
  if ((ret = tm.Open()) != Result::OK) {
    ROS_ERROR_STREAM("HLDS 3D TOF initialization file \"" << tm.inifilepath  <<"tof.ini\" invalid.");
    return -1;
  }

  // Get number of TOF sensor and TOF information list
  numoftof = tm.GetTofList(&ptofinfo);

  if (numoftof == 0) {
    ROS_ERROR_STREAM("No HLDS 3D TOF sensors found.");
    return -1;
  }

  // Create Tof instances for TOF sensors
  Tof tof;

  // Open all Tof instances (Set TOF information)
  string vfpga = "";
  string vosv = "";
  string vroot = "";
  if (tof.Open(ptofinfo[0]) != Result::OK){
    ROS_ERROR_STREAM("Error connecting to HLDS 3D TOF sensor.");
    return -1;
  }

  if (tof.GetVersion(&vfpga, &vosv, &vroot) == Result::OK) {
    ROS_INFO_STREAM("HLDS 3D TOF version" << ":" << vfpga.c_str() << ":" << vosv.c_str() << ":" << vroot.c_str() );
  }

  if (tof.tofinfo.tofver != TofVersion::TOFv2) {
      ROS_ERROR_STREAM("This application is only for TOFv2 sensor");
      return -1;
  }

  // Once Tof instances are started, TofManager is not necessary and closed
  if (tm.Close() != Result::OK){
      ROS_ERROR_STREAM("Error closing HLDS 3D TOF manager.");
      return -1;
  }

  //Set TOF sensor parameters
  bool edge_signal_cutoff;
  int location_x, location_y, location_z;
  int angle_x, angle_y, angle_z;
  int low_signal_cutoff, ir_gain;
  double far_signal_cutoff;
  std::string camera_pixel, distance_mode, frame_rate;

  nh.param("frame_id", frame_id, std::string("base_link"));
  nh.param("cloud_topic", cloud_topic, std::string("cloud"));

  nh.param("sensor_location_x", location_x, 0);
  nh.param("sensor_location_y", location_y, 0);
  nh.param("sensor_location_z", location_z, 0);
  nh.param("sensor_angle_x", angle_x, 0);
  nh.param("sensor_angle_y", angle_y, 0);
  nh.param("sensor_angle_z", angle_z, 0);

  nh.param("edge_signal_cutoff", edge_signal_cutoff, true);
  nh.param("low_signal_cutoff", low_signal_cutoff, 20);
  nh.param("far_signal_cutoff", far_signal_cutoff, 0.0);

  nh.param("camera_pixel", camera_pixel, std::string("320x240"));
  nh.param("ir_gain", ir_gain, 8);
  nh.param("distance_mode", distance_mode, std::string("dm_1_0x"));
  nh.param("frame_rate", frame_rate, std::string("fr30fps"));

  //Set camera mode
  if (tof.SetCameraMode(CameraMode::Depth_Ir) != Result::OK){
    ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set Camera Mode Error");
    return -1;
  }

  //Set camera pixel
  if(camera_pixel == "320x240") {
      if (tof.SetCameraPixel(CameraPixel::w320h240) != Result::OK){
        //  if (tof.SetCameraPixel(CameraPixel::w160h120) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set Camera Pixel Error");
        return -1;
      }
  }
  else if(camera_pixel == "160x120") {
        if (tof.SetCameraPixel(CameraPixel::w160h120) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set Camera Pixel Error");
        return -1;
      }
  }
  else if(camera_pixel == "80x60") {
        if (tof.SetCameraPixel(CameraPixel::w80h60) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set Camera Pixel Error");
        return -1;
      }
  }
  else if(camera_pixel == "64x48") {
        if (tof.SetCameraPixel(CameraPixel::w64h48) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set Camera Pixel Error");
        return -1;
      }
  }
  else if(camera_pixel == "40x30") {
        if (tof.SetCameraPixel(CameraPixel::w40h30) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set Camera Pixel Error");
        return -1;
      }
  }
  else if(camera_pixel == "32x24") {
        if (tof.SetCameraPixel(CameraPixel::w32h24) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set Camera Pixel Error");
        return -1;
      }
  }

  //Set distance mode
  if(distance_mode == "dm_2_0x") {
      if (tof.SetDistanceMode(DistanceMode::dm_2_0x) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set distance mode Error");
        return -1;
      }
  }
  else if(distance_mode == "dm_1_5x") {
      if (tof.SetDistanceMode(DistanceMode::dm_1_5x) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set distance mode Error");
        return -1;
      }
  }
  else if(distance_mode == "dm_1_0x") {
      if (tof.SetDistanceMode(DistanceMode::dm_1_0x) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set distance mode Error");
        return -1;
      }
  }
  else if(distance_mode == "dm_0_5x") {
      if (tof.SetDistanceMode(DistanceMode::dm_0_5x) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set distance mode Error");
        return -1;
      }
  }

  //Set frame rate
  if(frame_rate == "fr30fps") {
      if (tof.SetFrameRate(FrameRate::fr30fps) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set frame rate Error");
        return -1;
      }
  }
  else if(frame_rate == "fr16fps") {
      if (tof.SetFrameRate(FrameRate::fr16fps) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set frame rate Error");
        return -1;
      }
  }
  else if(frame_rate == "fr8fps") {
      if (tof.SetFrameRate(FrameRate::fr8fps) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set frame rate Error");
        return -1;
      }
  }
  else if(frame_rate == "fr4fps") {
      if (tof.SetFrameRate(FrameRate::fr4fps) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set frame rate Error");
        return -1;
      }
  }
  else if(frame_rate == "fr2fps") {
      if (tof.SetFrameRate(FrameRate::fr2fps) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set frame rate Error");
        return -1;
      }
  }
  else if(frame_rate == "fr1fps") {
      if (tof.SetFrameRate(FrameRate::fr1fps) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set frame rate Error");
        return -1;
      }
  }

  //Set IR gain
  if (tof.SetIrGain(ir_gain) != Result::OK){
    ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set Ir gain Error");
    return -1;
  }

  //Low signal cutoff
  if (tof.SetLowSignalCutoff(low_signal_cutoff) != Result::OK){
    ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Low Signal Cutoff Error");
    return -1;
  }

  //Far signal cutoff
  if (tof.SetFarSignalCutoff(far_signal_cutoff) != Result::OK){
    ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Far Signal Cutoff Error");
    return -1;
  }

  //Edge noise reduction
  if(edge_signal_cutoff == true){
      if (tof.SetEdgeSignalCutoff(EdgeSignalCutoff::Enable) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Edge Signal Cutoff Error");
        return -1;
      }
  }
  else if(edge_signal_cutoff == false){
      if (tof.SetEdgeSignalCutoff(EdgeSignalCutoff::Disable) != Result::OK){
        ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Edge Signal Cutoff Error");
        return -1;
      }
  }

  //Set physical installation position
  if (tof.SetAttribute(location_x, location_y, location_z, angle_x, angle_y, angle_z) != Result::OK){
    ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Set Camera Position Error");
    return -1;
  }

  //Start
  if (tof.Run() != Result::OK){
    ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Run Error");
    return -1;
  }

  //Create color table
  frame.CreateColorTable(0, 65530);

  // Initialize the ROS publisher
  cloudPublisher = nh.advertise<pcl::PointCloud<pcl::PointXYZ> > (cloud_topic, 1);
  irPublisher = nh.advertise<sensor_msgs::Image> ("/image_ir", 1);
  depthPublisher = nh.advertise<sensor_msgs::Image> ("/image_depth", 1);
  depthrawPublisher = nh.advertise<sensor_msgs::Image> ("/image_depth_raw", 1);
  ROS_INFO_STREAM("Publishing data on topic \"" << nh.resolveName(cloud_topic) << "\" with frame_id \"" << frame_id << "\"");

  ros::Rate r(40);

  //while (nh.ok ())
  while (ros::ok())
  {
    ros::spinOnce();

    //Get the latest frame number
    long frameno;
    TimeStamp timestamp;
    tof.GetFrameStatus(&frameno, &timestamp);
    if (frameno != frame.framenumber) {
      //Read a new frame only if frame number is changed(Old data is shown if it is not changed.)

      //Read a frame of depth data
      ret = Result::OK;
      ret = tof.ReadFrame(&frame, &frameir);
      if (ret != Result::OK) {
        ROS_INFO_STREAM("HLDS 3D TOF read frame error");
        break;
      }

      //3D conversion(with lens correction)
      frame3d.Convert(&frame);

      //3D rotation(to top view)
      frame3d.Rotate(angle_x, angle_y, angle_z);

      // Create the IR image
      msgIr = sensor_msgs::ImagePtr(new sensor_msgs::Image);
      msgIr->height = frameir.height;
      msgIr->width = frameir.width;
      msgIr->is_bigendian = false;
      msgIr->encoding = sensor_msgs::image_encodings::MONO8;
      msgIr->data.resize(frameir.pixel);

      // Create the Depth image
      msgDepth = sensor_msgs::ImagePtr(new sensor_msgs::Image);
      msgDepth->height = frame.height;
      msgDepth->width = frame.width;
      msgDepth->is_bigendian = false;
      msgDepth->encoding = sensor_msgs::image_encodings::RGB8;
      msgDepth->data.resize(frame.pixel*3);

      // Create the Depth Raw image
      msgDepthRaw = sensor_msgs::ImagePtr(new sensor_msgs::Image);
      msgDepthRaw->height = frame.height;
      msgDepthRaw->width = frame.width;
      msgDepthRaw->is_bigendian = false;
      msgDepthRaw->encoding = sensor_msgs::image_encodings::TYPE_8UC1;
      msgDepthRaw->data.resize(frame.pixel);

      // Create the point cloud
      pcl::PointCloud<pcl::PointXYZ>::Ptr ptCloud(new pcl::PointCloud<pcl::PointXYZ>());
      ptCloud->header.stamp = pcl_conversions::toPCL(ros::Time::now());
      ptCloud->header.frame_id = frame_id;
      ptCloud->width = frame3d.width;
      ptCloud->height = frame3d.height;
      ptCloud->is_dense = false;
      ptCloud->points.resize(sizeof(uint16_t)*frame3d.pixel);

      for (int i = 0; i < frame.pixel; i++) {
        pcl::PointXYZ &point = ptCloud->points[i];
        if ((frame.CalculateLength(frame.databuf[i]) >= frame.distance_min) &&
                (frame.CalculateLength(frame.databuf[i]) <= frame.distance_max)){

          //Get coordinates after 3D conversion
          point.x = frame3d.frame3d[i].x/1000.0;
          point.y = frame3d.frame3d[i].y/1000.0;
          point.z = frame3d.frame3d[i].z/1000.0;

          //Get IR frame data
          msgIr->data[i] = frameir.databuf[i]/256.0;

          //Get Depth frame data
          msgDepth->data[3*i] = frame.ColorTable[2][frame.databuf[i]]; //Red
          msgDepth->data[3*i+1] = frame.ColorTable[1][frame.databuf[i]]; //Green
          msgDepth->data[3*i+2] = frame.ColorTable[0][frame.databuf[i]]; //Blue

          //Get Depth Raw frame data
          msgDepthRaw->data[i] = frame.databuf[i]/256.0;
        }
        else {
            point.x = 0;
            point.y = 0;
            point.z = 0;

            msgIr->data[i] = 0;

            msgDepth->data[3*i] = 0;
            msgDepth->data[3*i+1] = 0;
            msgDepth->data[3*i+2] = 0;

            msgDepthRaw->data[i] = 0;
        }
      }

      // Publish topic
      cloudPublisher.publish(*ptCloud);
      irPublisher.publish(msgIr);
      depthPublisher.publish(msgDepth);
      depthrawPublisher.publish(msgDepthRaw);
    }
    r.sleep();
  }

  // Shutdown the sensor
  if (tof.Stop() != Result::OK || tof.Close() != Result::OK) {
    ROS_INFO_STREAM("HLDS 3D TOF ID " << tof.tofinfo.tofid << " Stop Error");
  }

  return 0;
}
