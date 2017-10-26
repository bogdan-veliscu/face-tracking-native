#ifndef VSDK_LANDMARKVERIFIER_H
#define VSDK_LANDMARKVERIFIER_H

#include "LBF.h"
#include "Predictor.h"
#include "FDP.h"

namespace VisageSDK
{
	class FDP;
	class LandmarkVerifier
	{
	public:
		LandmarkVerifier(const std::string& dataPath, bool doPackedLoading = true, bool useThresholds = false);
		~LandmarkVerifier();

		bool load(const std::string& dir, bool useThresholds);
		bool loadPacked(const std::string& filePath, bool useThresholds);
		bool isLoaded() const { return m_loaded; };
		void clear();

		void estimate(VsImage* gray, FDP* fdp, float faceSize) const;

		float calculateTrackingQuality(FDP* fdp, float yaw) const;

	private:
		void estimateForLandmark(VsImage* gray, LBF* landmarkLbf, Predictor* landmarkPredictor, FeaturePoint& fp, float faceSize, bool mirror) const;
		void interpolateQualityValues(FDP* fdp) const;

	private:
		std::vector<LBF*> m_lbfs;
		std::vector<Predictor*> m_predictors;
		std::vector<int> m_groupInds;
		std::vector<int> m_pointInds;
		bool m_loaded;
	};
}

#endif // VSDK_LANDMARKVERIFIER_H
