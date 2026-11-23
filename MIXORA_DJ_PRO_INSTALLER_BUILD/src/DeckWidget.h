#pragma once
#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QAudioDecoder>
#include <QAudioBuffer>
#include <QAudioDevice>
#include <QTimer>
#include <QVector>
#include <QUrl>
#include <array>
class QLabel;class QPushButton;class QSlider;class WaveformWidget;
class DeckWidget : public QWidget {
 Q_OBJECT
public:
 explicit DeckWidget(const QString& name,const QColor& color,QWidget* parent=nullptr);
 void loadFile(const QString& path);
 bool hasTrack() const;
 bool playing() const;
 double bpm() const{return m_bpm;}
 double playbackRate() const{return m_player.playbackRate();}
 qint64 position() const{return m_player.position();}
 qint64 duration() const{return m_player.duration();}
 qint64 cuePosition() const{return m_cueMs;}
 void setPlaybackRate(double rate);
 void setVolumeFactor(double factor);
 void setOutputDevice(const QAudioDevice& device);
 void setCueOutputDevice(const QAudioDevice& device);
 void setCueVolume(int percentage);
 void setCueEnabled(bool enabled);
 bool cueEnabled() const{return m_cueEnabled;}
 void seekToFraction(double fraction);
 void togglePlay();
 void syncTo(const DeckWidget* master);
 QString trackTitle() const;
signals:
 void trackLoaded(const QString& file);
 void statusMessage(const QString& message);
 void requestSync(DeckWidget* deck);
 void cueToggleRequested(DeckWidget* deck,bool enabled);
protected:
 void dragEnterEvent(QDragEnterEvent*) override;
 void dropEvent(QDropEvent*) override;
private:
 void updatePosition(qint64 ms);
 void updateDuration(qint64 ms);
 void analyzeAudio(const QString& filename);
 void collectBuffer();
 void finishAnalysis();
 void updatePlaybackVolume();
 void updateCueTransport();
 void reconcileCue();
 QString m_name,m_path;
 QMediaPlayer m_player;
 QAudioOutput m_output;
 QMediaPlayer m_cuePlayer;
 QAudioOutput m_cueOutput;
 QTimer m_cueTimer;
 QPushButton* m_cueMonitorButton=nullptr;
 bool m_cueEnabled=false;
 int m_cueVolume=65;
 QAudioDecoder m_decoder;
 WaveformWidget* m_wave=nullptr;
 QLabel *m_title=nullptr,*m_time=nullptr,*m_bpmLabel=nullptr,*m_rateLabel=nullptr;
 QPushButton *m_play=nullptr,*m_loop=nullptr;
 QSlider *m_pitch=nullptr,*m_gain=nullptr;
 QVector<float> m_peaks,m_energy;
 double m_analyzedSeconds=0;
 bool m_analysisPending=false;
 double m_bpm=0,m_externalVolume=1;
 qint64 m_cueMs=0,m_loopStart=0,m_loopLength=0;
 bool m_loopEnabled=false;
 std::array<qint64,4> m_hotcues{{-1,-1,-1,-1}};
};
