//
//  VoiceModuleManager.hpp
//
//  Created by Eric Kampman on 4/13/25.
//

#ifndef VoiceModuleManager_h
#define VoiceModuleManager_h

#include <vector>
#include <memory>
#include "pm_compat.h"
#include "pm_midi.h"

#include "ModuleFactory.hpp"
#include "Voice.hpp"
#include "PitchConverter.hpp"
#include "ConnectionCommandQueue.hpp"


#define LOG_VM(fmt, ...) \
    if (VoiceManager::loggingEnabled) \
        os_log_with_type(logVoiceManager, OS_LOG_TYPE_INFO, fmt, ##__VA_ARGS__)

constexpr size_t MAX_VOICES = 32;

class VoiceManager {
public:
	static constexpr size_t MaxVoices = MAX_VOICES;
		
	VoiceManager(const VoiceManager&) = delete;
	VoiceManager& operator=(const VoiceManager&) = delete;

	// Optionally allow moves if needed
//	VoiceManager(VoiceManager&&) = default;
//	VoiceManager& operator=(VoiceManager&&) = default;

	// ModuleFactory owns blockSize, sampleRate, but
	// putting ModuleFactory in the kernel was problematic.
	VoiceManager(double sampleRate = DEFAULT_SAMPLE_RATE,
                 uint32_t blockSize = MODULE_BUFF_SIZE);
	
	void initialize(double sampleRate);
	void initializeVoices(PitchConverter *pitchConverter);
    
    void reset();

private:
	void initializeModules(Voice& voice, PitchConverter *pitchConverter);
	void buildDefaultPatch(Voice& voice);

public:
	// MIDI Routing
	void handleMIDIMessage(const pm::UmpMessage& message);
	// to have VoiceManager choose the best Voice, use the below for noteOn.
	void noteOn(const pm::UmpMessage& msg);
	void noteOff(const pm::UmpMessage& msg);
	void allNotesOff();
    void panic();
    // The key of the sounding note that carries this host note id, or -1: for CLAP note
    // expressions that name only the id. Audio thread, like handleMIDIMessage.
    int keyForNoteId(int32_t noteId);
    // Whether any voice still holds this note-on (see VoiceStateManager::holdsNote). The plugin
    // reports CLAP_EVENT_NOTE_END for a host note once this turns false. Audio thread.
    bool holdsNote(const pm::UmpMessage& noteOn);

    // Sustain pedal
    void handleSustainPedalChange(bool newState);

    bool freeRunning();
    
    Module *getModule(AUParameterAddress paramAddress);
    // called from VoiceManagerBridge.
    void connectModules(uint64_t upstreamID, uint64_t downstreamID, AUParameterAddress inputAddress);
    void disconnectModule(uint64_t moduleID, int portID);

    // Point a destination mod-input at a source module's output (or disconnect if source == 0).
    // Clears whatever was on the input, then connects the new source, on all voices.
    void setModSource(AUParameterAddress destInput, AUParameterAddress source);

	// DSP block processing
    void process(uint32_t frameCount, float *left, float *right);
//	void process(uint32_t frameCount);
    
    // Parameters
    
    /* voice manager-specific params */
    uint32_t activeCount();
    AUValue voiceManagerParamAtIndex(size_t index);
    AUValue voiceManagerParamByEnum(NSUInteger paramEnum);
    void setVoiceManagerParameter(AUParameterAddress address, AUValue value);

    /* module params */
    void setParameter(AUParameterAddress address, AUValue value);
    AUValue getParameter(AUParameterAddress address);
    void clearInput(AUParameterAddress address);
    
    void updateSampleRate(double newSampleRate);
//    bool isVoiceManagerModule(AUParameterAddress address);

private:
	std::unique_ptr<ModuleFactory> moduleFactory;
    std::unique_ptr<Voice> referenceVoice;
	std::vector<std::unique_ptr<Voice>> voices;
	uint64_t voiceIdCounter = 0;

    PitchConverter pitchConverter;

    float params[MODULE_MAX_PARAMS];    // MIDI and global?

    static bool loggingEnabled;

    // Sustain pedal state
    bool sustainPedalActive = false;
    std::vector<uint32_t> pedalSustainedVoiceIDs;   // voice IDs with deferred NoteOff

    // Lock-free SPSC queue: main thread posts connection commands,
    // render thread drains them at the top of process().
    ConnectionCommandQueue<1024> commandQueue;
    void drainCommandQueue();

    // Pitch 7.25 as persistent per-key tuning (M2-104 §4.2.14.2): the last Pitch 7.25 sent for a
    // key, per group and channel, whether or not a note was sounding. A later note-on on that key
    // without a Pitch 7.9 attribute carries it (UmpMessage::keyPitch) and starts there. Per-Note
    // Management Reset clears the key. It is configuration a controller sent, like the bend ranges,
    // so reset() keeps it; the plugin state does not save it. CLAP tuning (UmpMessage::soundingOnly)
    // never writes it.
    struct KeyTuning { uint32_t pitch = 0; bool set = false; };   // pitch: Q7.25 semitones
    static constexpr size_t kTuningKeys = 16 * 16 * 128;
    std::vector<KeyTuning> keyTuning = std::vector<KeyTuning>(kTuningKeys);
    KeyTuning& keyTuningFor(const pm::UmpMessage& msg) {
        return keyTuning[((msg.group & 0x0Fu) * 16u + (msg.channel & 0x0Fu)) * 128u + (msg.note & 0x7Fu)];
    }
    pm::UmpMessage withKeyTuning(const pm::UmpMessage& noteOn);

	Voice* findFreeVoice();
	Voice* stealVoice(const pm::UmpMessage& msg);
	Voice* findVoiceForMessage(const pm::UmpMessage& msg);
	void releaseAllNotes(Voice& voice);
	void releaseNotesOnChannel(Voice& voice, uint8_t group, uint8_t channel);
	void dropPedalSustain(Voice& voice);
	// Channel mode messages, per MIDI: they act on the channel (and group) they arrive on.
	void allNotesOffOnChannel(uint8_t group, uint8_t channel);   // CC 123
	void allSoundOffOnChannel(uint8_t group, uint8_t channel);   // CC 120
    Voice* oldestVoice();
    Voice* lowestVoice();
    Voice* highestVoice();
    Voice* closest(const pm::UmpMessage& msg);
    Voice* furthest(const pm::UmpMessage& msg);
};

#endif /* VoiceModuleManager_h */
