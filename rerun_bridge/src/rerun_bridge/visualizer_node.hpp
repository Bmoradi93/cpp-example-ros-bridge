#pragma once

#include <map>
#include <string>

#include <ros/ros.h>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <yaml-cpp/yaml.h>
#include <rerun.hpp>

class RerunLoggerNode {
  public:
    RerunLoggerNode();
    void spin();

  private:
    std::map<std::string, std::string> _topic_to_entity_path;
    std::map<std::string, ros::Subscriber> _topic_to_subscriber;
    std::map<std::string, std::string> _tf_frame_to_entity_path;
    std::map<std::string, std::string> _tf_frame_to_parent;

    void _read_yaml_config(std::string yaml_path);

    std::string _resolve_entity_path(const std::string& topic) const;

    void _add_tf_tree(const YAML::Node& node, const std::string& parent_entity_path, const std::string& parent_frame);

    const rerun::RecordingStream _rec{"rerun_logger_node"};
    ros::NodeHandle _nh{"~"};
    std::string _root_frame;
    float _tf_fixed_rate;
    tf2_ros::Buffer _tf_buffer;
    tf2_ros::TransformListener _tf_listener{_tf_buffer};
    
    // Timestamp normalization
    mutable double _time_offset;
    mutable bool _time_offset_initialized;
    double _normalize_timestamp(const ros::Time& stamp) const;

    void _create_subscribers();
    void _update_tf() const;

    /* Message specific subscriber factory functions */
    ros::Subscriber _create_image_subscriber(const std::string& topic);
    ros::Subscriber _create_imu_subscriber(const std::string& topic);
    ros::Subscriber _create_pose_stamped_subscriber(const std::string& topic);
    ros::Subscriber _create_tf_message_subscriber(const std::string& topic);
    ros::Subscriber _create_odometry_subscriber(const std::string& topic);
    ros::Subscriber _create_camera_info_subscriber(const std::string& topic);
};
