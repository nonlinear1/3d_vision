BAG_NAME=/media/chamo/095d3ecf-bef8-469d-86a3-fe170aec49db/iphone_office/office/05-21-18-02-25.bag
OUT_ADDR=/media/chamo/095d3ecf-bef8-469d-86a3-fe170aec49db/iphone_office/merge_try1
EXE_ROOT=/home/chamo/Documents/work/3d_vision/visual_map

BAG_EXTRACT_ADDR=${EXE_ROOT}/devel/lib/bag_tool/bag_tool_exe
VOC_ADDR=${EXE_ROOT}/devel/lib/vslam/create_vol
ORB_SLAM_ADDR=${EXE_ROOT}/devel/lib/vslam/mono_kitti
INDEX_ADDR=${EXE_ROOT}/devel/lib/create_desc_index/create_desc_index_test
OPTI_ADDR=${EXE_ROOT}/devel/lib/optimizer_tool/optimizer_tool_test
SEG_ADDR=${EXE_ROOT}/devel/lib/seg_traj/seg_traj

#mkdir ${OUT_ADDR}/images
#${BAG_EXTRACT_ADDR} ${BAG_NAME} ${OUT_ADDR} img imu gps

#${VOC_ADDR} ${OUT_ADDR}
#${ORB_SLAM_ADDR} ${OUT_ADDR}/small_voc.yml ${OUT_ADDR}/vslam.yaml ${BAG_NAME} ${OUT_ADDR} img
#${SEG_ADDR} ${OUT_ADDR}
${OPTI_ADDR} ${OUT_ADDR}
#${INDEX_ADDR} ${OUT_ADDR}
