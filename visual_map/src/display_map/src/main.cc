#include "visualization/color-palette.h"
#include "visualization/color.h"
#include "visualization/common-rviz-visualization.h"
#include "read_write_data_lib/read_write.h"
#include "visual_map/visual_map.h"
#include "visual_map/visual_map_seri.h"

void show_mp_as_cloud(std::vector<Eigen::Vector3d>& mp_posis, std::string topic){
    Eigen::Matrix3Xd points;
    points.resize(3,mp_posis.size());
    for(int i=0; i<mp_posis.size(); i++){
        points.block<3,1>(0,i)=mp_posis[i];
    }    
    publish3DPointsAsPointCloud(points, visualization::kCommonRed, 1.0, visualization::kDefaultMapFrame,topic);
}
    
void show_pose_as_marker(std::vector<Eigen::Vector3d>& posis, std::vector<Eigen::Quaterniond>& rots, std::string topic){
    visualization::PoseVector poses_vis;
    for(int i=0; i<posis.size(); i=i+1){
        visualization::Pose pose;
        pose.G_p_B = posis[i];
        pose.G_q_B = rots[i];

        pose.id =poses_vis.size();
        pose.scale = 0.2;
        pose.line_width = 0.02;
        pose.alpha = 1;
        poses_vis.push_back(pose);
    }
    visualization::publishVerticesFromPoseVector(poses_vis, visualization::kDefaultMapFrame, "vertices", topic);
}
          
int main(int argc, char* argv[]){
    ros::init(argc, argv, "vis_map");
    ros::NodeHandle nh;
    visualization::RVizVisualizationSink::init();
    std::string res_root=argv[1];

    vm::VisualMap map;
    vm::loader_visual_map(map, res_root);
    std::vector<Eigen::Vector3d> traj_posi;
    std::vector<Eigen::Vector3d> mp_posi;
    for(int i=0; i<map.frames.size(); i++){
        traj_posi.push_back(map.frames[i]->position);
    }
    for(int i=0; i<map.mappoints.size(); i++){
        mp_posi.push_back(map.mappoints[i]->position);
    }
    show_mp_as_cloud(traj_posi, "vm_frame_posi");
    show_mp_as_cloud(mp_posi, "vm_mp_posi");
    ros::Rate loop_rate(10);
    for(int i=0; i<map.frames.size(); i++){
        visualization::LineSegmentVector matches;
        double t_proj_err=0;
        int mp_count=0;
        for(int j=0; j<map.frames[i]->obss.size(); j++){
            if(map.frames[i]->obss[j]!=nullptr){
                visualization::LineSegment line_segment;
                line_segment.from = map.frames[i]->position;
                line_segment.scale = 0.03;
                line_segment.alpha = 0.6;

                line_segment.color.red = 255;
                line_segment.color.green = 255;
                line_segment.color.blue = 255;
                line_segment.to = map.frames[i]->obss[j]->position;
                matches.push_back(line_segment);
                Eigen::Matrix<double, 3,4> proj_mat = map.frames[i]->getProjMat();
                Eigen::Vector4d posi_homo;
                posi_homo.block(0,0,3,1)=map.frames[i]->obss[j]->position;
                posi_homo(3)=1;
                Eigen::Vector3d proj_homo = proj_mat*posi_homo;
                //std::cout<<proj_mat<<std::endl;
                double u=proj_homo(0)/proj_homo(2);
                double v=proj_homo(1)/proj_homo(2);
                cv::Point2f uv= map.frames[i]->kps[j].pt;
                //std::cout<<u<<":"<<v<<"     "<<uv.x<<":"<<uv.y<<std::endl;
                
                float proj_err=sqrt((uv.x-u)*(uv.x-u)+(uv.y-v)*(uv.y-v));
                
                t_proj_err=t_proj_err+proj_err;
                mp_count++;
            }
        }
        std::cout<<"avg. proj. err.: "<<t_proj_err/mp_count<<std::endl;
        visualization::publishLines(matches, 0, visualization::kDefaultMapFrame,visualization::kDefaultNamespace, "vm_matches");
        if(ros::ok()){
            loop_rate.sleep();
        }else{
            break;
        }
    }
    
    
    ros::spin();

    return 0;
}