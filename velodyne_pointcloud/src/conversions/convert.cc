/*
 *  Copyright (C) 2009, 2010 Austin Robot Technology, Jack O'Quin
 *  Copyright (C) 2011 Jesse Vera
 *  Copyright (C) 2012 Austin Robot Technology, Jack O'Quin
 *  License: Modified BSD Software License Agreement
 *
 *  $Id$
 */

/** @file

    This class converts raw Velodyne 3D LIDAR packets to PointCloud2.

*/

#include "convert.h"

namespace velodyne_pointcloud
{
  /** @brief Constructor. */
  Convert::Convert(ros::NodeHandle node, ros::NodeHandle private_nh):
    data_(new velodyne_rawdata::RawData())
  {
    private_nh.param("npackets", config_.npackets,
                     velodyne_rawdata::PACKETS_PER_REV);
    ROS_INFO_STREAM("number of packets to accumulate: " << config_.npackets);

    data_->setup(private_nh);

    // allocate space for the output point cloud data
    pc_.points.reserve(config_.npackets*velodyne_rawdata::SCANS_PER_PACKET);
    pc_.points.clear();
    pc_.width = 0;
    pc_.height = 1;
    pc_.is_dense = true;
    packetCount_ = 0;

    // advertise output point cloud (before subscribing to input data)
    output_ =
      node.advertise<sensor_msgs::PointCloud2>("velodyne/pointcloud2", 10);

    // subscribe to VelodyneScan packets
    velodyne_scan_ =
      node.subscribe("velodyne/packets", 10,
                     &Convert::processScan, (Convert *) this,
                     ros::TransportHints().tcpNoDelay(true));
  }

  /** @brief Callback for raw scan messages. */
  void Convert::processScan(const velodyne_msgs::VelodyneScan::ConstPtr &scanMsg)
  {
    if (output_.getNumSubscribers() == 0)         // no one listening?
      return;                                     // avoid much work

    // allocate a point cloud with same time and frame ID as raw data
    velodyne_rawdata::VPointCloud::Ptr
      outMsg(new velodyne_rawdata::VPointCloud());
    outMsg->header.stamp = scanMsg->header.stamp;
    outMsg->header.frame_id = scanMsg->header.frame_id;
    outMsg->height = 1;

    // process each packet provided by the driver
    for (size_t i = 0; i < scanMsg->packets.size(); ++i)
      {
        data_->unpack(scanMsg->packets[i], outMsg);
      }

    // publish an empty point cloud message (test scaffolding)
    ROS_DEBUG_STREAM("Publishing " << outMsg->height * outMsg->width
                     << " Velodyne points, time: " << outMsg->header.stamp);
    output_.publish(outMsg);
  }

} // namespace velodyne_pointcloud