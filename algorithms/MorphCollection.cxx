#ifndef __ALGORITHMS_MORPHCOLLECTION_CXX__
#define __ALGORITHMS_MORPHCOLLECTION_CXX__

#include "MorphCollection.h"


std::vector<std::vector<float>> algorithms::MorphCollection::removeCoherentNoise(
                         std::vector<std::vector<float>>& filteredWaveforms,
                         const unsigned int grouping, 
                         const unsigned int nTicks,
                         const unsigned int structuringElement)
{
  std::vector<std::vector<float>> waveLessCoherent(
    filteredWaveforms.size(), std::vector<float>(filteredWaveforms.at(0).size(), 0.0));

  std::vector<std::vector<bool>> selectVals(
    filteredWaveforms.size(), std::vector<bool>(filteredWaveforms.at(0).size(), 0.0));

  getSelectVals(filteredWaveforms, grouping, nTicks, structuringElement, selectVals);

  auto numChannels = (int) filteredWaveforms.size();

  for (unsigned int i=0; i<nTicks; ++i) {
    for (unsigned int j=0; j<numChannels/grouping; ++j) {
      int group_start = j * grouping;
      int group_end = (j+1) * grouping;
      // Compute median.
      std::vector<float> v;
      v.resize(grouping);
      short counter = 0;
      for (auto c=group_start; c<group_end; ++c) {
        if (selectVals[c][i]) {
          v[counter] = filteredWaveforms[c][i];
        } else {
          v[counter] = 0.0;
        }
        ++counter;
      }
      float median = 0.0;
      if (v.size() % 2 == 0) {
        const auto m1 = v.begin() + v.size() / 2 - 1;
        const auto m2 = v.begin() + v.size() / 2;
        std::nth_element(v.begin(), m1, v.end());
        const auto e1 = *m1;
        std::nth_element(v.begin(), m2, v.end());
        const auto e2 = *m2;
        median = (e1 + e2) / 2.0;
      } else {
        const auto m = v.begin() + v.size() / 2;
        std::nth_element(v.begin(), m, v.end());
        median = *m;
      }
      for (auto c=group_start; c<group_end; ++c) {
        waveLessCoherent[c][i] = filteredWaveforms[c][i] - median;
      }
    }
  }
  return waveLessCoherent;
}

void algorithms::MorphCollection::getSelectVals(const std::vector<std::vector<float>>& waveforms,
                                                const unsigned int grouping,
                                                const unsigned int nTicks,
                                                const unsigned int structuringElement,
                                                std::vector<std::vector<bool>>& selectVals)
{
  WaveformUtils wUtils;
  auto numChannels = waveforms.size();
  selectVals.resize(numChannels);

  for (auto i=0; i<numChannels; ++i) {
    std::vector<float> dilation;
    wUtils.getDilation(waveforms[i], structuringElement, dilation);
    float dilationMed = 0.0;
    if (dilation.size() % 2 == 0) {
      const auto m1 = dilation.begin() + dilation.size() / 2 - 1;
      const auto m2 = dilation.begin() + dilation.size() / 2;
      std::nth_element(dilation.begin(), m1, dilation.end());
      const auto e1 = *m1;
      std::nth_element(dilation.begin(), m2, dilation.end());
      const auto e2 = *m2;
      dilationMed = (e1 + e2) / 2.0;
    } else {
      const auto m = dilation.begin() + dilation.size() / 2;
      std::nth_element(dilation.begin(), m, dilation.end());
      dilationMed = *m;
    }
    std::vector<float> dilationBase;
    dilationBase.resize(dilation.size());
    for (auto i=0; i<dilationBase.size(); ++i) {
      dilationBase[i] = dilation[i] - dilationMed;
    }
    float dilationRMS;
    dilationRMS = std::sqrt(std::inner_product(dilationBase.begin(), 
      dilationBase.end(), dilationBase.begin(), 0.) / float(dilationBase.size()));
    float threshold;
    threshold = dilationMed + dilationRMS * 2.5;
    for (auto j=0; j<nTicks; ++j) {
      bool sVal = true;
      if (waveforms[i][j] >= threshold) {
        sVal = false;
      }
      selectVals[i][j] = sVal;
    }
  }
  return;
}


void algorithms::MorphCollection::filterWaveforms(const std::vector<std::vector<short>>& waveforms,
                                               const unsigned int grouping,
                                               const unsigned int nTicks,
                                               const unsigned int structuringElement,
                                               std::vector<std::vector<float>>& noiseRemovedWfs,
                                               std::vector<float>& means,
                                               std::vector<float>& medians,
                                               std::vector<float>& rmss)
{
  auto numChannels = waveforms.size();
  std::vector<std::vector<float>> filteredWaveforms;
  filteredWaveforms.resize(numChannels);
  means.resize(numChannels);
  medians.resize(numChannels);
  rmss.resize(numChannels);

  for (unsigned int i=0; i<numChannels; ++i) {
    float mean = 0;
    float median = 0;
    float mode = 0;
    float rms = 0.0;
    float skewness = 0.0;
    filteredWaveforms[i].resize(waveforms.at(0).size());
    WaveformUtils wUtils;
    wUtils.getWaveformParams(waveforms[i], mean, median, mode, skewness, rms);
    // Subtract Pedestals (Median of given waveform, one channel)
    std::transform(waveforms[i].begin(),waveforms[i].end(),filteredWaveforms[i].begin(),std::bind(std::minus<float>(),std::placeholders::_1,median));
    means[i] = mean;
    medians[i] = median;
    rmss[i] = rms;
  }
  noiseRemovedWfs = removeCoherentNoise(filteredWaveforms, grouping, nTicks, structuringElement);
  return;
}

#endif
