#include "rerun_bridge/rerun_ros_interface.hpp"
#include "collection_adapters.hpp"

#include <cv_bridge/cv_bridge.h>
#include <ros/ros.h>
#include <rerun.hpp>

void log_imu(
    const rerun::RecordingStream& rec, const std::string& entity_path,
    const sensor_msgs::Imu::ConstPtr& msg, double normalized_timestamp
) {
    rec.set_time_seconds("timestamp", normalized_timestamp);

    rec.log(entity_path + "/x", rerun::Scalar(msg->linear_acceleration.x));
    rec.log(entity_path + "/y", rerun::Scalar(msg->linear_acceleration.y));
    rec.log(entity_path + "/z", rerun::Scalar(msg->linear_acceleration.z));
}

void log_image(
    const rerun::RecordingStream& rec, const std::string& entity_path,
    const sensor_msgs::Image::ConstPtr& msg, double normalized_timestamp
) {
    rec.set_time_seconds("timestamp", normalized_timestamp);

    // Depth images are 32-bit float (in meters) or 16-bit uint (in millimeters)
    // See: https://ros.org/reps/rep-0118.html
    if (msg->encoding == "16UC1") {
        cv::Mat img = cv_bridge::toCvCopy(msg)->image;
        rec.log(
            entity_path,
            rerun::DepthImage({img.rows, img.cols}, rerun::TensorBuffer::u16(img)).with_meter(1000)
        );
    } else if (msg->encoding == "32FC1") {
        // NOTE this has not been tested
        cv::Mat img = cv_bridge::toCvCopy(msg)->image;
        rec.log(
            entity_path,
            rerun::DepthImage({img.rows, img.cols}, rerun::TensorBuffer::f32(img)).with_meter(1.0)
        );
    } else {
        cv::Mat img = cv_bridge::toCvCopy(msg, "rgb8")->image;
        rec.log(
            entity_path,
            rerun::Image(tensor_shape(img), rerun::TensorBuffer::u8(img))
        );
    }
}

void log_pose_stamped(
    const rerun::RecordingStream& rec, const std::string& entity_path,
    const geometry_msgs::PoseStamped::ConstPtr& msg, double normalized_timestamp
) {
    rec.set_time_seconds("timestamp", normalized_timestamp);

    rec.log(
        entity_path,
        rerun::Transform3D(
            rerun::Vector3D(msg->pose.position.x, msg->pose.position.y, msg->pose.position.z),
            rerun::Quaternion::from_wxyz(
                msg->pose.orientation.w,
                msg->pose.orientation.x,
                msg->pose.orientation.y,
                msg->pose.orientation.z
            )
        )
    );

    // this is a somewhat hacky way to get a trajectory visualization in Rerun
    // this should be be easier in the future, see https://github.com/rerun-io/rerun/issues/723
    std::string trajectory_entity_path = "/trajectories/" + entity_path;
    rec.log(
        trajectory_entity_path,
        rerun::Points3D(
            {{static_cast<float>(msg->pose.position.x),
              static_cast<float>(msg->pose.position.y),
              static_cast<float>(msg->pose.position.z)}}
        )
    );
}

void log_tf_message(
    const rerun::RecordingStream& rec,
    const std::map<std::string, std::string>& tf_frame_to_entity_path,
    const tf2_msgs::TFMessage::ConstPtr& msg, double normalized_timestamp
) {
    for (const auto& transform : msg->transforms) {
        if (tf_frame_to_entity_path.find(transform.child_frame_id) ==
            tf_frame_to_entity_path.end()) {
            ROS_WARN("No entity path for frame_id %s, skipping", transform.child_frame_id.c_str());
            continue;
        }

        rec.set_time_seconds("timestamp", normalized_timestamp);

        rec.log(
            tf_frame_to_entity_path.at(transform.child_frame_id),
            rerun::Transform3D(
                rerun::Vector3D(
                    transform.transform.translation.x,
                    transform.transform.translation.y,
                    transform.transform.translation.z
                ),
                rerun::Quaternion::from_wxyz(
                    transform.transform.rotation.w,
                    transform.transform.rotation.x,
                    transform.transform.rotation.y,
                    transform.transform.rotation.z
                )
            )
        );
    }
}

void log_odometry(
    const rerun::RecordingStream& rec, const std::string& entity_path,
    const nav_msgs::Odometry::ConstPtr& msg, double normalized_timestamp
) {
    rec.set_time_seconds("timestamp", normalized_timestamp);

    rec.log(
        entity_path,
        rerun::Transform3D(
            rerun::Vector3D(
                msg->pose.pose.position.x,
                msg->pose.pose.position.y,
                msg->pose.pose.position.z
            ),
            rerun::Quaternion::from_wxyz(
                msg->pose.pose.orientation.w,
                msg->pose.pose.orientation.x,
                msg->pose.pose.orientation.y,
                msg->pose.pose.orientation.z
            )
        )
    );
}

void log_camera_info(
    const rerun::RecordingStream& rec, const std::string& entity_path,
    const sensor_msgs::CameraInfo::ConstPtr& msg, double normalized_timestamp
) {
    // Rerun uses column-major order for Mat3x3
    const std::array<float, 9> image_from_camera = {
        static_cast<float>(msg->K[0]),
        static_cast<float>(msg->K[3]),
        static_cast<float>(msg->K[6]),
        static_cast<float>(msg->K[1]),
        static_cast<float>(msg->K[4]),
        static_cast<float>(msg->K[7]),
        static_cast<float>(msg->K[2]),
        static_cast<float>(msg->K[5]),
        static_cast<float>(msg->K[8]),
    };
    rec.log(
        entity_path,
        rerun::Pinhole(image_from_camera)
            .with_resolution(static_cast<int>(msg->width), static_cast<int>(msg->height))
    );
}

void log_transform(
    const rerun::RecordingStream& rec, const std::string& entity_path,
    const geometry_msgs::TransformStamped& msg, double normalized_timestamp
) {
    rec.set_time_seconds("timestamp", normalized_timestamp);

    rec.log(
        entity_path,
        rerun::Transform3D(
            rerun::Vector3D(
                msg.transform.translation.x,
                msg.transform.translation.y,
                msg.transform.translation.z
            ),
            rerun::Quaternion::from_wxyz(
                msg.transform.rotation.w,
                msg.transform.rotation.x,
                msg.transform.rotation.y,
                msg.transform.rotation.z
            )
        )
    );
}
