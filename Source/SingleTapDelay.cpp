#include <iostream>
#include <vector>
#include <JuceHeader.h>

class Circularbuff {
public:
   Circularbuff(int size) : buffer(size, 0.0f), writepos(0), buffersize(size) {}


   float process(float input, int delaysamples) {
       buffer[writepos] = input;


       int readpos = (writepos - delaysamples + buffersize) % buffersize;


       float buffoutput = buffer[readpos];


       writepos = (writepos + 1) % buffersize;


       return buffoutput;
   }


private:
std::vector<float> buffer;
int writepos;
int buffersize;


};


void delay(std::vector<float>& inputbuff, std::vector<float>& outputbuff, int delaysamples, float mix) {


   Circularbuff delayline(delaysamples + 1);


   for (size_t i = 0; i < inputbuff.size(); i++) {
       float delayedsig = delayline.process(inputbuff[i], delaysamples);


       outputbuff[i] = (1.0f - mix) * (inputbuff[i] + (mix * delayedsig));


   }
}


float timetosamples(float time, int samplerate) {
   return time * samplerate;
}


int main() {
   //IO File paths
   std::string inputwav = "/Users/apple/Desktop/Spring25/granBasics/input.wav";
   std::string outputwav = "/Users/apple/Desktop/Spring25/granBasics/output.wav";


   //Creating JUCE File objects
   juce::File inputfile(inputwav);
   juce::File outputfile(outputwav);


   //Registering basic audio formats
   juce::AudioFormatManager formatManager;
   formatManager.registerBasicFormats();


   float delaytime = 0.5f;
   int samplerate = 48000;
   int delaysamples = static_cast<int>(timetosamples(delaytime, samplerate));


   //Creating a unique pointer for the reader
   std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(inputfile));


   if (!reader) {
       std::cout << "Failed to open input file." << std::endl;
       return 1;
   }


   samplerate = static_cast<int>(reader->sampleRate);
   // Recalculate delay based on actual file
   delaysamples = static_cast<int>(delaytime * samplerate);


   int numChannels = reader->numChannels;
   std::vector<Circularbuff> delaybuff;


   for (int i = 0; i < numChannels; ++i) {
       delaybuff.emplace_back(Circularbuff(delaysamples + 1));
   }


   //Creating the JUCE Audio Buffer
   juce::AudioBuffer<float> buffer(numChannels, static_cast<int>(reader->lengthInSamples));


   //Reading data into the buffer
   reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true);


   //Processing audio with the delay effect
   for (int channel = 0; channel < buffer.getNumChannels(); channel++) {
       float* channeldata = buffer.getWritePointer(channel);
       Circularbuff& channeldelay = delaybuff[channel];


       for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
           float drysample = channeldata[sample];


           float delayedsample = channeldelay.process(drysample, delaysamples);


           float wetsample = 0.7f * drysample + 0.3f * delayedsample;
           channeldata[sample] = wetsample;
       }
   }


   // Creating a unique pointer for the writer
   std::unique_ptr<juce::AudioFormatWriter> writer(
       formatManager.findFormatForFileExtension("wav")->createWriterFor(
           new juce::FileOutputStream(outputfile),
           reader->sampleRate,
           reader->numChannels,
           16,
           {},
           0));


   if (!writer) {
       std::cout << "Failed to create writer." << std::endl;
       return 1;
   }


   writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());


   std::cout << "Delay processing complete." << std::endl;
   return 0;


}
