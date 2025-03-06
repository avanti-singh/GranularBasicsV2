#include <iostream>
#include <vector>
#include <cmath>
#include <JuceHeader.h>

class CircularBuffer {
public:
    CircularBuffer(int size, int sampleRate) 
        : buffer(size * sampleRate, 0.0f), writePos(0), bufferSize(size * sampleRate) {}
    
    void write(float sample) {
        buffer[writePos] = sample;
        writePos = (writePos + 1) % bufferSize;
    }

    float readSample(int delaySamples) const {
        if (delaySamples >= bufferSize) return 0.0f;  // Prevent out-of-bounds read
        int readPos = (writePos - delaySamples + bufferSize) % bufferSize;
        return buffer[readPos];
    }

    int getBufferSize() const { return bufferSize; }

private:
    std::vector<float> buffer;
    int writePos;
    int bufferSize;
};

class Grain {
public:
    Grain(int start, int duration, CircularBuffer* delayBuffer)
        : startSample(start), grainDuration(duration), delayBuffer(delayBuffer), currentSample(0) {}

    bool process(float& output) {
        if (currentSample >= grainDuration) return false;

        // Envelope (linear fade-in & fade-out)
        float env;
        int attack = grainDuration / 4;
        int release = grainDuration / 4;
        if (currentSample < attack) {
            env = static_cast<float>(currentSample) / attack;
        } else if (currentSample > grainDuration - release) {
            env = static_cast<float>(grainDuration - currentSample) / release;
        } else {
            env = 1.0f;
        }

        // Read from buffer safely
        float sample = delayBuffer->readSample(currentSample);
        output += sample * env * 0.5f; // Reduce gain to prevent clipping
        currentSample++;
        return true;
    }

private:
    int startSample;
    int grainDuration;
    CircularBuffer* delayBuffer;
    int currentSample;
};

class GranularSynth {
public:
    GranularSynth(int sampleRate, int bufferSize, float grainSize, float overlap)
        : sampleRate(sampleRate), grainSize(grainSize), overlap(overlap), delayBuffer(bufferSize, sampleRate) {}

    void process(const std::vector<float>& input, std::vector<float>& output) {
        int hopSize = static_cast<int>(grainSize * (1.0f - overlap) * sampleRate);
        int grainDuration = static_cast<int>(grainSize * sampleRate);

        for (size_t i = 0; i < input.size(); i++) {
            delayBuffer.write(input[i]);
            if (i % hopSize == 0) {
                grains.emplace_back(i, grainDuration, &delayBuffer);
            }
        }

        for (size_t i = 0; i < output.size(); i++) {
            float grainOutput = 0.0f;
            for (auto it = grains.begin(); it != grains.end(); ) {
                if (!it->process(grainOutput)) {
                    it = grains.erase(it);
                } else {
                    ++it;
                }
            }
            output[i] += grainOutput;  // Properly accumulate grains
        }
    }

private:
    int sampleRate;
    float grainSize;
    float overlap;
    CircularBuffer delayBuffer;
    std::vector<Grain> grains;
};

int main() {
    std::string inputwav = "/Users/apple/Desktop/Spring25/granBasics/input.wav";
    std::string outputwav = "/Users/apple/Desktop/Spring25/granBasics/out4.wav";
    
    juce::File inputfile(inputwav);
    juce::File outputfile(outputwav);
    
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(inputfile));
    if (!reader) {
        std::cout << "Failed to open input file." << std::endl;
        return 1;
    }

    int sampleRate = static_cast<int>(reader->sampleRate);
    int numChannels = reader->numChannels;
    int numSamples = static_cast<int>(reader->lengthInSamples);
    
    juce::AudioBuffer<float> buffer(numChannels, numSamples);
    reader->read(&buffer, 0, numSamples, 0, true, true);
    
    std::vector<std::vector<float>> inputBuffers(numChannels, std::vector<float>(numSamples));
    std::vector<std::vector<float>> outputBuffers(numChannels, std::vector<float>(numSamples, 0.0f));

    for (int ch = 0; ch < numChannels; ch++) {
        for (int i = 0; i < numSamples; i++) {
            inputBuffers[ch][i] = buffer.getSample(ch, i);
        }
    }

    for (int ch = 0; ch < numChannels; ch++) {
        GranularSynth granular(sampleRate, sampleRate, 0.05f, 0.4f);
        granular.process(inputBuffers[ch], outputBuffers[ch]);
    }

    for (int ch = 0; ch < numChannels; ch++) {
        for (int i = 0; i < numSamples; i++) {
            buffer.setSample(ch, i, outputBuffers[ch][i]);
        }
    }
    
    std::unique_ptr<juce::FileOutputStream> outputStream = std::make_unique<juce::FileOutputStream>(outputfile);
    if (!outputStream->openedOk()) {
        std::cout << "Failed to open output file for writing." << std::endl;
        return 1;
    }

    std::unique_ptr<juce::AudioFormatWriter> writer(
        formatManager.findFormatForFileExtension("wav")->createWriterFor(
            outputStream.release(),
            reader->sampleRate,
            reader->numChannels,
            16,
            {},
            0));

    if (!writer || !writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples())) {
        std::cout << "Failed to write output file." << std::endl;
        return 1;
    }

    std::cout << "Granular Synthesis complete." << std::endl;
    return 0;
}
