#pragma once
#include <vector>
#include <algorithm>
#include <cstdlib>

namespace core_dsp::arpeggiator {
    
    enum class Mode {
        Up = 0,
        Down,
        UpDown,
        Random
    };

    struct Arpeggiator {
        Mode mode = Mode::Up;
        
        void noteOn(int note, float velocity) {
            auto it = std::find(heldNotes.begin(), heldNotes.end(), note);
            if (it == heldNotes.end()) {
                heldNotes.push_back(note);
                std::sort(heldNotes.begin(), heldNotes.end());
            }
            lastVelocity = velocity;
        }
        
        void noteOff(int note) {
            auto it = std::find(heldNotes.begin(), heldNotes.end(), note);
            if (it != heldNotes.end()) {
                heldNotes.erase(it);
            }
        }
        
        void reset() {
            heldNotes.clear();
            currentIndex = -1;
            direction = 1;
        }
        
        bool isActive() const {
            return !heldNotes.empty();
        }
        
        int getNextNote() {
            if (heldNotes.empty()) return -1;
            if (heldNotes.size() == 1) return heldNotes[0];
            
            if (mode == Mode::Up) {
                currentIndex++;
                if (currentIndex >= (int)heldNotes.size()) currentIndex = 0;
            } 
            else if (mode == Mode::Down) {
                // Se estivermos saindo de um UpDown ou Random, corrigimos o índice
                if (currentIndex == -1) currentIndex = (int)heldNotes.size();
                currentIndex--;
                if (currentIndex < 0) currentIndex = (int)heldNotes.size() - 1;
            }
            else if (mode == Mode::UpDown) {
                currentIndex += direction;
                if (currentIndex >= (int)heldNotes.size()) {
                    currentIndex = (int)heldNotes.size() - 2;
                    if (currentIndex < 0) currentIndex = 0;
                    direction = -1;
                } else if (currentIndex < 0) {
                    currentIndex = 1;
                    if (currentIndex >= (int)heldNotes.size()) currentIndex = 0;
                    direction = 1;
                }
            }
            else if (mode == Mode::Random) {
                currentIndex = std::rand() % heldNotes.size();
            }
            
            // Safety clamp
            if (currentIndex < 0) currentIndex = 0;
            if (currentIndex >= (int)heldNotes.size()) currentIndex = (int)heldNotes.size() - 1;
            
            return heldNotes[currentIndex];
        }
        
        float getLastVelocity() const { return lastVelocity; }
        
    private:
        std::vector<int> heldNotes;
        int currentIndex = -1;
        int direction = 1;
        float lastVelocity = 0.8f;
    };
}
