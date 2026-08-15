//
// Copyright (c) 2019-2025 yanggaofeng
//
#include <yangcapture/YangVideoCapture.h>
#include <yangutil/sys/YangString.h>

void yang_get_camera_indexs(std::vector<int>* pvs, std::string camIdx) {
	std::vector<std::string> result = yang_split(camIdx, ',');

	for(size_t i = 0; i < result.size(); i++) {
		pvs->push_back(atoi(result[i].c_str()));
	}
}

YangVideoCapture::YangVideoCapture() {
	m_camIdx = 0;
	m_isStart = 0;
	m_para = NULL;
}

YangVideoCapture::~YangVideoCapture() {
	m_para = NULL;
}

void YangVideoCapture::run() {
	m_isStart = 1;
	this->startLoop();
	m_isStart = 0;
}

void YangVideoCapture::stop() {
	this->stopLoop();
}
