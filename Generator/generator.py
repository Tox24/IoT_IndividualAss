import numpy as np
import sounddevice as sd

sps = 44100
freq_hz = 1000
duration_s = 3.0
attenuation = 0.5

each_sample_number = np.arange(duration_s * sps)

waveform = np.sin(2 * np.pi * each_sample_number * freq_hz / sps)
waveform_quiet = waveform * attenuation

sd.play(waveform_quiet, sps, device = 'ESP32-OTG')
sd.wait()