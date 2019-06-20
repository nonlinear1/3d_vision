#include <loc_lib/ChamoLoc.h>
#ifndef __APPLE__
#include "visualization/color-palette.h"
#include "visualization/color.h"
#include "visualization/common-rviz-visualization.h"
#endif

#include <ctime>

struct Match{
        int query_desc_id;
        int frame_id_tar;
        int desc_id_tar;
        int track_id;
        bool operator==(const Match& other) const {
            bool result = true;
            result &= query_desc_id == other.query_desc_id;
            result &= frame_id_tar == other.frame_id_tar;
            result &= desc_id_tar == other.desc_id_tar;
            result &= track_id == other.track_id;
            return result;
        }
};

namespace std {
    template <>
    struct hash<Match> {
        std::size_t operator()( const Match& value) const {
            const std::size_t h0(std::hash<int>()(value.query_desc_id));
            const std::size_t h1(std::hash<int>()(value.frame_id_tar));
            const std::size_t h2(std::hash<int>()(value.desc_id_tar));
            const std::size_t h3(std::hash<int>()(value.track_id));
            return h0 ^ h1 ^ h2 ^ h3;
        }
    };

    template <>
    struct hash<std::pair<int, int>> {
        std::size_t operator()(const std::pair<int, int>& value)
            const {
                const std::size_t h1(std::hash<int>()(value.first));
                const std::size_t h2(std::hash<int>()(value.second));
            return h1 ^ h2;
        }
    };
}


namespace wayz {
#ifndef __APPLE__
    
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
#endif
    
    void convert_mat_eigen(Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic>& matrix, cv::Mat mat){
        //Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> m_temp;
        matrix.resize(mat.rows, mat.cols);
        for(int i=0; i<mat.rows; i++){
            for(int j=0; j<mat.cols; j++){
                //std::cout<<(int)mat.at<unsigned char>(i, j)<<",";
                matrix(i, j) = mat.at<unsigned char>(i, j);
            }
        }
        matrix.transposeInPlace();
    }

    void convert_mat_float_eigen_double(Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>& matrix, cv::Mat mat){
        matrix.resize(mat.rows, mat.cols);
        for(int i=0; i<mat.rows; i++){
            for(int j=0; j<mat.cols; j++){
                matrix(i, j) = mat.at<double>(i, j);
            }
        }
    }

    void convert_eigen_double_mat_float(const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>& matrix, cv::Mat& mat){
        mat = cv::Mat(matrix.rows(), matrix.cols(), CV_32FC1);
        for(int i=0; i<matrix.rows(); i++){
            for(int j=0; j<matrix.cols(); j++){
                mat.at<float>(i, j)=matrix(i, j);
            }
        }
    }

    void ChamoLoc::StartLocalization(const std::string& filename){
    };
    
    int ChamoLoc::UpdateByMap(cv::Mat Img, double timestamp, std::vector<cv::Point3f>& inliers_mp, std::vector<cv::Point2f>& inliers_kp){
        cv::Mat desc_list;
        std::vector<cv::KeyPoint> kps_list;
        std::vector<std::vector<std::vector<std::size_t>>> mGrid;
        orb_slam::ExtractOrb(Img, desc_list, kps_list, mGrid, cam_inter_cv, cam_distort_cv);
        int final_match_count=0;
            
        std::map<int, int> match_count_per_frame;
        std::unordered_map<int, std::vector<Match>> frame_to_match;
        for(int i=0; i<desc_list.rows; i++){
            Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> desc_eigen;
            convert_mat_eigen(desc_eigen, desc_list.row(i));
            Eigen::VectorXf projected_desc;
            descriptor_projection::ProjectDescriptor(desc_eigen, projection_matrix_, 10, projected_desc);
            //std::cout<<desc_list.row(i)<<std::endl;
            //std::cout<<desc_eigen.transpose().cast<int>()<<std::endl;
            //std::cout<<projected_desc.transpose()<<std::endl;
            Eigen::VectorXi out_indices;
            int condi_count=3;
            out_indices.resize(condi_count);
            Eigen::VectorXf out_distances;
            out_distances.resize(condi_count);
            Eigen::Matrix<float, 10 ,1> projected_desc_fix;
            projected_desc_fix=projected_desc;
            index_->GetNNearestNeighbors(projected_desc_fix, condi_count, out_indices, out_distances);
            //std::cout<<out_indices.transpose()<<std::endl;
            std::cout<<out_distances.transpose()<<std::endl;
            for(int j=0; j<out_indices.size(); j++){
                
                if(out_indices[j]!=-1){
                    //
                    int frame_index = index_->get_desc_frameid(out_indices[j]);
                    int track_index = index_->get_desc_trackid(out_indices[j]);
                    //std::cout<<frame_index<<":"<<track_index<<std::endl;
                    Match match;
                    match.desc_id_tar=out_indices[j];
                    match.frame_id_tar=frame_index;
                    match.query_desc_id=i;
                    match.track_id=track_index;
                    if(frame_to_match.find(match.frame_id_tar)==frame_to_match.end()){
                        std::vector<Match> matches;
                        matches.push_back(match);
                        frame_to_match[match.frame_id_tar]=matches;
                    }else{
                        frame_to_match[match.frame_id_tar].push_back(match);
                    }

                    if(match_count_per_frame.count(frame_index)!=0){
                        match_count_per_frame[frame_index]++;
                    }else{
                        match_count_per_frame[frame_index]=1;
                    }
                }
            }
            
        }
        
        typedef int ComponentId;
        constexpr ComponentId kInvalidComponentId = -1;
        typedef std::unordered_map<Match, ComponentId> MatchesToComponents;
        typedef std::unordered_map<ComponentId, std::unordered_set<Match>> Components;
        typedef std::unordered_map<int, std::vector<Match>> LandmarkMatches;

        MatchesToComponents matches_to_components;
        LandmarkMatches landmark_matches;

        for (const std::pair<int, std::vector<Match>>& id_matches_pair : frame_to_match) {
            for (const Match& match : id_matches_pair.second) {
                landmark_matches[match.track_id].emplace_back(match);
                matches_to_components.emplace(match, kInvalidComponentId);
            }
        }

        ComponentId count_component_index = 0;
        size_t max_component_size = 0u;
        ComponentId max_component_id = kInvalidComponentId;
        Components components;
        for (const MatchesToComponents::value_type& match_to_component : matches_to_components) {
            if (match_to_component.second != kInvalidComponentId)
                continue;
            ComponentId component_id = count_component_index++;

            // Find the largest set of keyframes connected by landmark covisibility.
            std::queue<Match> exploration_queue;
            exploration_queue.push(match_to_component.first);
            while (!exploration_queue.empty()) {
                const Match& exploration_match = exploration_queue.front();

                if (match_count_per_frame[exploration_match.frame_id_tar]<5) {
                    exploration_queue.pop();
                    continue;
                }

                const MatchesToComponents::iterator exploration_match_and_component = matches_to_components.find(exploration_match);
                CHECK(exploration_match_and_component != matches_to_components.end());

                if (exploration_match_and_component->second == kInvalidComponentId) {
                    // Not part of a connected component.
                    exploration_match_and_component->second = component_id;
                    components[component_id].insert(exploration_match);
                    // Mark all observations (which are matches) from this ID (keyframe or
                    // vertex) as visited.
                    if(frame_to_match.count(exploration_match.frame_id_tar)==0){
                        return false;
                    }
                    const std::vector<Match>& id_matches = frame_to_match[exploration_match.frame_id_tar];
                    for (const Match& id_match : id_matches) {
                        matches_to_components[id_match] = component_id;
                        components[component_id].insert(id_match);

                        // Put all observations of this landmark on the stack.
                        const std::vector<Match>& lm_matches =
                            landmark_matches[id_match.track_id];
                        for (const Match& lm_match : lm_matches) {
                            if (matches_to_components[lm_match] == kInvalidComponentId) {
                                exploration_queue.push(lm_match);
                            }
                        }
                    }

                    if (components[component_id].size() > max_component_size) {
                        max_component_size = components[component_id].size();
                        max_component_id = component_id;
                    }
                }
                exploration_queue.pop();
            }
        }
        
        std::map<int, std::vector<Match>> frame_matches;
        // Only store the structure matches if there is a relevant amount of them.
        int total_match_count=0;
        
        //std::cout<<"max_component_size: "<<max_component_size<<std::endl;
        
        if (max_component_size > 10) {
            
            const std::unordered_set<Match>& matches_max_component = components[max_component_id];
            typedef std::pair<int, int> KeypointLandmarkPair;
            std::unordered_set<KeypointLandmarkPair> used_matches;
            used_matches.reserve(2u * matches_max_component.size());
            for (const Match& structure_match : matches_max_component) {
                const bool is_match_unique = used_matches.emplace(structure_match.desc_id_tar, structure_match.track_id).second;
                //std::cout<<structure_match.desc_id_tar<<":"<<structure_match.track_id<<std::endl;
                if (!is_match_unique) {
                // Skip duplicate (keypoint to landmark) structure matches.
                    continue;
                }
                frame_matches[structure_match.frame_id_tar].push_back(structure_match);
                total_match_count=total_match_count+1;
            }

            std::vector<cv::Point3f> point3ds;
            std::vector<cv::Point2f> point2ds;
            for (auto item :frame_matches){
                for(int i=0; i<item.second.size(); i++){
                    cv::Point2f pt=kps_list[item.second[i].query_desc_id].pt;
                    point2ds.push_back(pt);
                    cv::Point3f posi;
                    posi.x= mp_posis[item.second[i].track_id].x();
                    posi.y= mp_posis[item.second[i].track_id].y();
                    posi.z= mp_posis[item.second[i].track_id].z();
                    point3ds.push_back(posi);
                }
                //std::cout << "["<<item.first<<": "<<item.second.size()<<"]";
            }
            //std::cout<<std::endl;
            cv::Mat rvec;
            cv::Mat tvec;
            cv::Mat inliers;
            cv::Mat cam_distort_zero=cam_distort_cv.clone();
            cam_distort_zero.at<float>(0)=0;
            cam_distort_zero.at<float>(1)=0;
            cam_distort_zero.at<float>(2)=0;
            cam_distort_zero.at<float>(3)=0;
            std::cout<<"point3ds: "<<point3ds.size()<<std::endl;
            cv::solvePnPRansac(point3ds, point2ds, cam_inter_cv, cam_distort_zero, rvec, tvec, false, 1000, 2.0f, 0.99, inliers, cv::SOLVEPNP_EPNP);
            std::cout<<"inliers.rows: "<<inliers.rows<<std::endl;
            if(inliers.rows<20){
                return final_match_count;
            }
            final_match_count=inliers.rows;
            for(int i=0; i<inliers.rows; i++){
                //std::cout<<inliers.at<int>(i)<<std::endl;
                inliers_kp.push_back(point2ds[inliers.at<int>(i)]);
                inliers_mp.push_back(point3ds[inliers.at<int>(i)]);
            }
            
            cv::Mat rot_m;
            cv::Rodrigues(rvec, rot_m);
            Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> rot_m_eigen;
            Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> tvec_eigen;
            convert_mat_float_eigen_double(rot_m_eigen, rot_m);
            convert_mat_float_eigen_double(tvec_eigen, tvec);
            Eigen::Matrix4d pose_inv=Eigen::Matrix4d::Identity();
            pose_inv.block(0,0,3,3)=rot_m_eigen;
            pose_inv.block(0,3,3,1)=tvec_eigen;
            Eigen::Matrix4d pose_eigen=pose_inv.inverse();
            Eigen::Quaterniond pose_qua(pose_eigen.block<3,3>(0,0));
            
            posi_match_vec.push_back(pose_eigen.block(0,3,3,1));
            rot_match_vec.push_back(pose_qua);
        }
        return final_match_count;
    }
    void ChamoLoc::Debug_Image_pose(const double timestamp,const int camera_id, cv::Mat& img_distort)
    {
//         if (!init_state_.isInitialized() || img_distort.empty()) {
//             return;
//         }
//         cv::Mat Img;
//         cv::undistort(img_distort, Img, cam_inter_cv, cam_distort_cv);
//         
//         if(posi_list.size()!=0){
//             double msgTime = timestamp;
//             if (msgTime != imgUpdateMeas_.template get<mtImgMeas::_aux>().imgTime_) {
//                 for (int i = 0; i < FilterType::mtState::nCam_; i++) {
//                     if (imgUpdateMeas_.template get<mtImgMeas::_aux>().isValidPyr_[i]) {
//                         std::cout
//                             << "    \033[31mFailed Synchronization of Camera Frames, t = "
//                             << msgTime << "\033[0m" << std::endl;
//                     }
//                 }
//                 imgUpdateMeas_.template get<mtImgMeas::_aux>().reset(msgTime);
//             }
//             imgUpdateMeas_.template get<mtImgMeas::_aux>().pyr_[camera_id].computeFromImage(Img, true);
//             imgUpdateMeas_.template get<mtImgMeas::_aux>().isValidPyr_[camera_id] = true;
// 
//             bool measurement_accepted = false;
//             if (imgUpdateMeas_.template get<mtImgMeas::_aux>().areAllValid()) {
//                 measurement_accepted = mpFilter_->template addUpdateMeas<0>(imgUpdateMeas_, msgTime);
//                 imgUpdateMeas_.template get<mtImgMeas::_aux>().reset(msgTime);
//                 //std::thread::id this_id = std::this_thread::get_id();
//             }
//         }
//         std::vector<cv::Point3f> inliers_mp;
//         std::vector<cv::Point2f> inliers_kp;
//         bool loc_re = UpdateByMap(Img, timestamp, inliers_mp, inliers_kp);
//         updateFilter();
//         
//         Eigen::Matrix4d t_mb=Eigen::Matrix4d::Identity();
//         t_mb.block(0,3,3,1)=mpFilter_->safe_.state_.WrWM();
//         t_mb.block(0,0,3,3)=MPD(mpFilter_->safe_.state_.qWM()).matrix();
//         Eigen::Matrix4d t_wm=Eigen::Matrix4d::Identity();
//         t_wm.block(0,3,3,1)=mpFilter_->safe_.state_.poseLin(0);
//         t_wm.block(0,0,3,3)=MPD(mpFilter_->safe_.state_.poseRot(0)).matrix();
//         
//         Eigen::Matrix4d t_re=t_mb;
//         posi_vec.push_back(t_re.block(0,3,3,1));
//         posi_list[timestamp]=t_re.block(0,3,3,1);
//         Eigen::Matrix3d rot_eigen = t_re.block(0,0,3,3);
//         Eigen::Quaterniond rot_q(rot_eigen);
//         rot_list[timestamp]=rot_q;
//         timestamp_list.push_back(timestamp);
//         posi_loc_vec.push_back(t_re.block(0,3,3,1));
//         rot_loc_vec.push_back(rot_q);
//         
//         if(loc_re){
//             cv::Mat debug_img;
//             cv::cvtColor(Img, debug_img, cv::COLOR_GRAY2RGB);
//             for(int i=0; i<inliers_kp.size(); i++){
//                 cv::circle(debug_img, inliers_kp[i], 4, CV_RGB(0,0,255), 2);
//             }
// 
//             img_distort = debug_img.clone();
//             //visualization::RVizVisualizationSink::publish("match_img", debug_img);
//             //visualization::LineSegmentVector matches;
//             for(int i=0; i<inliers_mp.size(); i++){
//                 visualization::LineSegment line_segment;
//                 line_segment.from = posi_match_vec.back();
//                 line_segment.scale = 0.03;
//                 line_segment.alpha = 0.6;
// 
//                 line_segment.color.red = 255;
//                 line_segment.color.green = 255;
//                 line_segment.color.blue = 255;
//                 Eigen::Vector3d mp_posi_eig;
//                 mp_posi_eig(0)=inliers_mp[i].x;
//                 mp_posi_eig(1)=inliers_mp[i].y;
//                 mp_posi_eig(2)=inliers_mp[i].z;
//                 line_segment.to = mp_posi_eig;
// 
//                 fea_match_vec.push_back(mp_posi_eig);
//                 //std::cout<<line_segment.to.transpose()<<std::endl;
//                 //std::cout<<posi_match_vec.back()<<std::endl;
//                 //matches.push_back(line_segment);
//             }
//             //visualization::publishLines(matches, 0, visualization::kDefaultMapFrame,visualization::kDefaultNamespace, "map_match1");
//         }
        
    }
    void ChamoLoc::Debug_Feature_pose(std::vector<Eigen::Vector3d>& vec)
    {
        if (fea_match_vec.size() > 0)
        {
            vec.swap(fea_match_vec);

            std::vector<Eigen::Vector3d>().swap(fea_match_vec);
        }
    }
    void ChamoLoc::Export_Raw_MatchFile(std::string& path)
    {
        std::ofstream outfile_raw;
        outfile_raw.open(path + "/raw_match.txt");
        std::vector<raw_match>::iterator time_matchnum_itor = time_matchnum_vec.begin();
        for(;time_matchnum_itor != time_matchnum_vec.end();time_matchnum_itor++)
        {
            std::stringstream raw_info;
            raw_info << std::setprecision(16) << time_matchnum_itor->timestamp << "," ;
            raw_info << time_matchnum_itor->gmatchnum << ",";
            raw_info << time_matchnum_itor->runtime << std::endl;
            outfile_raw << raw_info.str();
        }
        
        outfile_raw.close();
    }
    void ChamoLoc::AddImage(const double timestamp,const int camera_id, const cv::Mat& img_distort)
    {
        clock_t start = clock();
        cv::Mat Img;
        cv::undistort(img_distort, Img, cam_inter_cv, cam_distort_cv);
        std::vector<cv::Point3f> inliers_mp;
        std::vector<cv::Point2f> inliers_kp;
        int match_count = UpdateByMap(Img, timestamp, inliers_mp, inliers_kp);
#ifndef __APPLE__
        if(match_count){
            show_mp_as_cloud(posi_match_vec, "loc_lib_posi");
            cv::Mat debug_img;
            cv::cvtColor(Img, debug_img, cv::COLOR_GRAY2RGB);
            for(int i=0; i<inliers_kp.size(); i++){
                cv::circle(debug_img, inliers_kp[i], 4, CV_RGB(0,0,255), 2);
            }
            visualization::RVizVisualizationSink::publish("loc_lib_debug_img", debug_img);
            visualization::LineSegmentVector matches;
            for(int i=0; i<inliers_mp.size(); i++){
                visualization::LineSegment line_segment;
                line_segment.from = posi_match_vec.back();
                line_segment.scale = 0.03;
                line_segment.alpha = 0.6;

                line_segment.color.red = 255;
                line_segment.color.green = 255;
                line_segment.color.blue = 255;
                Eigen::Vector3d mp_posi_eig;
                mp_posi_eig(0)=inliers_mp[i].x;
                mp_posi_eig(1)=inliers_mp[i].y;
                mp_posi_eig(2)=inliers_mp[i].z;
                line_segment.to = mp_posi_eig;
                //std::cout<<line_segment.to.transpose()<<std::endl;
                //std::cout<<posi_match_vec.back()<<std::endl;
                matches.push_back(line_segment);
            }
            visualization::publishLines(matches, 0, visualization::kDefaultMapFrame,visualization::kDefaultNamespace, "loc_lib_match_line");
        }
#endif
        double dur = (double)(clock() - start)/CLOCKS_PER_SEC;

        raw_match su = {0};
        su.runtime = dur;
        su.timestamp = timestamp;
        su.gmatchnum = match_count;

        time_matchnum_vec.push_back(su);
    };
    
    void ChamoLoc::AddIMU(const double time_s, const Eigen::Vector3d& Accl, const Eigen::Vector3d& Gyro){
        return;
        
    };
    void ChamoLoc::AddMap(const std::string& folder_path){
        std::ifstream in_stream(folder_path+"/words_projmat.dat", std::ios_base::binary);
        int deserialized_version;
        common::Deserialize(&deserialized_version, &in_stream);
        int serialized_target_dimensionality;
        Eigen::MatrixXf words_first_half_;
        Eigen::MatrixXf words_second_half_;
        common::Deserialize(&serialized_target_dimensionality, &in_stream);
        common::Deserialize(&projection_matrix_, &in_stream);
        common::Deserialize(&words_first_half_, &in_stream);
        common::Deserialize(&words_second_half_, &in_stream);

        index_.reset(new loop_closure::inverted_multi_index::InvertedMultiIndex<5>(words_first_half_, words_second_half_, 10));
        loop_closure::proto::InvertedMultiIndex proto_inverted_multi_index;
        std::fstream input(folder_path+"/index.dat", std::ios::in | std::ios::binary);
        if (!proto_inverted_multi_index.ParseFromIstream(&input)) {
            std::cerr << "Failed to parse map data." << std::endl;
        }
        index_->deserialize(proto_inverted_multi_index);
        
        //feature points
        std::string posi_addr=folder_path+"/mp_posi_opt.txt";
        CHAMO::read_mp_posi(posi_addr, mp_posis);
        show_mp_as_cloud(mp_posis, "temp_mp");
        
        std::string cam_addr=folder_path+"/camera_config.txt";
        CHAMO::read_cam_info(cam_addr, cam_inter, cam_distort, Tbc);
        convert_eigen_double_mat_float(cam_inter, cam_inter_cv);
        convert_eigen_double_mat_float(cam_distort, cam_distort_cv);
    };
    
    void ChamoLoc::Shutdown(){
        
    };

    bool ChamoLoc::Debug_QueryPose(const double timestamp, Eigen::Vector3d& Pos, Eigen::Quaterniond& Ori)
    {
        if (posi_match_vec.empty())
        {
            std::cout << "[fail][Debug_QueryPose] posi_loc_queue is empty!" << std::endl;
            return false;
        }
        if (rot_loc_vec.empty())
        {
            std::cout << "[fail][Debug_QueryPose] rot_loc_queue is empty!" << std::endl;
            return false;
        }

        Pos = posi_match_vec.back();
        Ori = rot_loc_vec.back();

        std::vector<Eigen::Vector3d>().swap(posi_match_vec);
        std::vector<Eigen::Quaterniond>().swap(rot_loc_vec);

        return true;
    };

    bool ChamoLoc::QueryPose(const double timestamp, Eigen::Vector3d& Pos, Eigen::Vector3d& Vel, Eigen::Quaterniond& Ori) const{
        if(posi_match_vec.size()>0){
            Pos=(--posi_list.end())->second;
            return true;
        }else{
            return false;
        }
    };
    
}
