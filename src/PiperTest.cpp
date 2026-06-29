/*
  Piper TTS test tool
  Copyright (C) 2026  Lukáš Karas

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// Minimal command line tool that synthesizes a text into a WAV file using the
// vendored Piper TTS stack (libpiper + espeak-ng + onnxruntime).
//
// Usage:
//   PiperTest <voice.onnx> <espeak-ng-data-dir> <output.wav> <text...>
//
// The voice config is assumed to be <voice.onnx>.json (libpiper default when
// the config path is NULL).

#include <piper.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

// Write a little-endian integer of given byte width into a stream.
void writeLE(std::ostream &out, uint32_t value, int bytes)
{
  for (int i = 0; i < bytes; ++i) {
    char b = static_cast<char>((value >> (8 * i)) & 0xFF);
    out.write(&b, 1);
  }
}

// Write a canonical 16-bit PCM mono WAV file from float samples in [-1, 1].
bool writeWav(const std::string &path,
              const std::vector<float> &samples,
              int sampleRate)
{
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    std::cerr << "Cannot open output file: " << path << std::endl;
    return false;
  }

  const uint16_t channels = 1;
  const uint16_t bitsPerSample = 16;
  const uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
  const uint16_t blockAlign = channels * (bitsPerSample / 8);
  const uint32_t dataSize = static_cast<uint32_t>(samples.size()) * (bitsPerSample / 8);

  out.write("RIFF", 4);
  writeLE(out, 36 + dataSize, 4);
  out.write("WAVE", 4);

  out.write("fmt ", 4);
  writeLE(out, 16, 4);              // PCM header size
  writeLE(out, 1, 2);               // audio format = PCM
  writeLE(out, channels, 2);
  writeLE(out, sampleRate, 4);
  writeLE(out, byteRate, 4);
  writeLE(out, blockAlign, 2);
  writeLE(out, bitsPerSample, 2);

  out.write("data", 4);
  writeLE(out, dataSize, 4);

  for (float s : samples) {
    if (s > 1.0f)  s = 1.0f;
    if (s < -1.0f) s = -1.0f;
    int16_t pcm = static_cast<int16_t>(s * 32767.0f);
    writeLE(out, static_cast<uint16_t>(pcm), 2);
  }

  return out.good();
}

} // namespace

int main(int argc, char *argv[])
{
  if (argc < 5) {
    std::cerr << "Usage:\n  " << argv[0]
              << " <voice.onnx> <espeak-ng-data-dir> <output.wav> <text...>"
              << std::endl;
    return 1;
  }

  const std::string modelPath = argv[1];
  const std::string espeakData = argv[2];
  const std::string outputPath = argv[3];

  std::string text;
  for (int i = 4; i < argc; ++i) {
    if (i > 4) {
      text += " ";
    }
    text += argv[i];
  }

  std::cout << "Model:       " << modelPath << std::endl;
  std::cout << "espeak data: " << espeakData << std::endl;
  std::cout << "Output:      " << outputPath << std::endl;
  std::cout << "Text:        " << text << std::endl;

  piper_synthesizer *synth = piper_create(modelPath.c_str(),
                                          nullptr, // <model>.json
                                          espeakData.c_str());
  if (synth == nullptr) {
    std::cerr << "Failed to create Piper synthesizer" << std::endl;
    return 2;
  }

  piper_synthesize_options options = piper_default_synthesize_options(synth);

  if (piper_synthesize_start(synth, text.c_str(), &options) != PIPER_OK) {
    std::cerr << "Failed to start synthesis" << std::endl;
    piper_free(synth);
    return 3;
  }

  std::vector<float> samples;
  int sampleRate = 22050;

  piper_audio_chunk chunk;
  int rc = PIPER_OK;
  while ((rc = piper_synthesize_next(synth, &chunk)) != PIPER_DONE) {
    if (rc != PIPER_OK) {
      std::cerr << "Synthesis error, code " << rc << std::endl;
      piper_free(synth);
      return 4;
    }
    sampleRate = chunk.sample_rate;
    samples.insert(samples.end(),
                   chunk.samples,
                   chunk.samples + chunk.num_samples);
  }

  piper_free(synth);

  std::cout << "Synthesized " << samples.size() << " samples at "
            << sampleRate << " Hz" << std::endl;

  if (!writeWav(outputPath, samples, sampleRate)) {
    std::cerr << "Failed to write WAV file" << std::endl;
    return 5;
  }

  std::cout << "Wrote " << outputPath << std::endl;
  return 0;
}

