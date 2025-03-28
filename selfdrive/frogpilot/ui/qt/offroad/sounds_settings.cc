#include <filesystem>

#include "selfdrive/frogpilot/ui/qt/offroad/sounds_settings.h"

void playSound(const std::string &alert, int volume) {
  std::string stockPath = "/data/openpilot/selfdrive/assets/sounds/" + alert + ".wav";
  std::string themePath = "/data/openpilot/selfdrive/frogpilot/assets/active_theme/sounds/" + alert + ".wav";

  std::string filePath;
  if (std::filesystem::exists(themePath)) {
    filePath = themePath;
  } else {
    filePath = stockPath;
  }

  std::system("pkill -f 'ffplay'");
  std::system(("ffplay -nodisp -autoexit -volume " + std::to_string(std::clamp(volume, 0, 100)) + " \"" + filePath + "\"").c_str());
}

FrogPilotSoundsPanel::FrogPilotSoundsPanel(FrogPilotSettingsWindow *parent) : FrogPilotListWidget(parent), parent(parent) {
  QStackedLayout *soundsLayout = new QStackedLayout();
  addItem(soundsLayout);

  FrogPilotListWidget *soundsList = new FrogPilotListWidget(this);

  FrogPilotListWidget *alertVolumeControlList = new FrogPilotListWidget(this);
  FrogPilotListWidget *customAlertsList = new FrogPilotListWidget(this);

  ScrollView *soundsPanel = new ScrollView(soundsList, this);
  soundsLayout->addWidget(soundsPanel);

  ScrollView *alertVolumeControlPanel = new ScrollView(alertVolumeControlList, this);
  soundsLayout->addWidget(alertVolumeControlPanel);
  ScrollView *customAlertsPanel = new ScrollView(customAlertsList, this);
  soundsLayout->addWidget(customAlertsPanel);

  const std::vector<std::tuple<QString, QString, QString, QString>> soundsToggles {
    {"AlertVolumeControl", tr("Alert Volume Controller"), tr("Control the volume level for each individual sound in openpilot."), "../frogpilot/assets/toggle_icons/icon_mute.png"},
    {"DisengageVolume", tr("Disengage Volume"), tr("Related alerts:\n\nAdaptive Cruise Disabled\nBrake Pedal Pressed\nParking Brake Engaged\nSpeed too Low"), ""},
    {"EngageVolume", tr("Engage Volume"), tr("Related alerts:\n\nNNFF Torque Controller loaded\nopenpilot engaged"), ""},
    {"PromptVolume", tr("Prompt Volume"), tr("Related alerts:\n\nCar Detected in Blindspot\nSteer Unavailable Below \"X\"\nSpeed too Low\nTake Control, Turn Exceeds Steering Limit"), ""},
    {"PromptDistractedVolume", tr("Prompt Distracted Volume"), tr("Related alerts:\n\nPay Attention, Driver Distracted\nTouch Steering Wheel, Driver Unresponsive"), ""},
    {"RefuseVolume", tr("Refuse Volume"), tr("Related alerts:\n\nopenpilot Unavailable"), ""},
    {"WarningSoftVolume", tr("Warning Soft Volume"), tr("Related alerts:\n\nBRAKE!, Risk of Collision\nTAKE CONTROL IMMEDIATELY"), ""},
    {"WarningImmediateVolume", tr("Warning Immediate Volume"), tr("Related alerts:\n\nDISENGAGE IMMEDIATELY, Driver Distracted\nDISENGAGE IMMEDIATELY, Driver Unresponsive"), ""},

    {"CustomAlerts", tr("Custom Alerts"), tr("Custom FrogPilot alerts for openpilot events."), "../frogpilot/assets/toggle_icons/icon_green_light.png"},
    {"GoatScream", tr("Goat Scream Steering Saturated Alert"), tr("Enable the famed \"Goat Scream\" that has brought both joy and anger to FrogPilot users all around the world!"), ""},
    {"GreenLightAlert", tr("Green Light Alert"), tr("Play an alert when the traffic light changes from red to green."), ""},
    {"LeadDepartingAlert", tr("Lead Departing Alert"), tr("Play an alert when the lead vehicle starts starts to depart."), ""},
    {"LoudBlindspotAlert", tr("Loud Blindspot Vehicle Alert"), tr("Play a loud alert for when a vehicle is detected in the blindspot when attempting to change lanes."), ""},
    {"SpeedLimitChangedAlert", tr("Speed Limit Changed Alert"), tr("Play an alert when the speed limit changes."), ""},
  };

  for (const auto &[param, title, desc, icon] : soundsToggles) {
    AbstractControl *soundsToggle;

    if (param == "AlertVolumeControl") {
      FrogPilotManageControl *alertVolumeControlToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(alertVolumeControlToggle, &FrogPilotManageControl::manageButtonClicked, [soundsLayout, alertVolumeControlPanel]() {
        soundsLayout->setCurrentWidget(alertVolumeControlPanel);
      });
      soundsToggle = alertVolumeControlToggle;
    } else if (alertVolumeControlKeys.find(param) != alertVolumeControlKeys.end()) {
      std::map<float, QString> volumeLabels;
      for (int i = 0; i <= 101; ++i) {
        volumeLabels[i] = i == 0 ? tr("Muted") : i == 101 ? tr("Auto") : QString::number(i) + "%";
      }
      std::vector<QString> alertButton{"Test"};
      if (param == "WarningImmediateVolume") {
        soundsToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, 25, 101, QString(), volumeLabels, 1, true, {}, alertButton, false, false);
      } else {
        soundsToggle = new FrogPilotParamValueButtonControl(param, title, desc, icon, 0, 101, QString(), volumeLabels, 1, true, {}, alertButton, false, false);
      }

    } else if (param == "CustomAlerts") {
      FrogPilotManageControl *customAlertsToggle = new FrogPilotManageControl(param, title, desc, icon);
      QObject::connect(customAlertsToggle, &FrogPilotManageControl::manageButtonClicked, [soundsLayout, customAlertsPanel]() {
        soundsLayout->setCurrentWidget(customAlertsPanel);
      });
      soundsToggle = customAlertsToggle;

    } else {
      soundsToggle = new ParamControl(param, title, desc, icon);
    }

    toggles[param] = soundsToggle;

    if (alertVolumeControlKeys.find(param) != alertVolumeControlKeys.end()) {
      alertVolumeControlList->addItem(soundsToggle);
    } else if (customAlertsKeys.find(param) != customAlertsKeys.end()) {
      customAlertsList->addItem(soundsToggle);
    } else {
      soundsList->addItem(soundsToggle);

      parentKeys.insert(param);
    }

    if (FrogPilotManageControl *frogPilotManageToggle = qobject_cast<FrogPilotManageControl*>(soundsToggle)) {
      QObject::connect(frogPilotManageToggle, &FrogPilotManageControl::manageButtonClicked, this, &FrogPilotSoundsPanel::openParentToggle);
    }

    QObject::connect(soundsToggle, &AbstractControl::showDescriptionEvent, [this]() {
      update();
    });
  }

  for (const QString &key : alertVolumeControlKeys) {
    FrogPilotParamValueButtonControl *toggle = static_cast<FrogPilotParamValueButtonControl*>(toggles[key]);
    QObject::connect(toggle, &FrogPilotParamValueButtonControl::buttonClicked, [this, key, toggle]() {
      toggle->updateParam();

      updateFrogPilotToggles();

      util::sleep_for(UI_FREQ);

      QString alertKey = key;
      alertKey.remove("Volume");

      QString snakeCaseKey;
      for (int i = 0; i < alertKey.size(); ++i) {
        QChar c = alertKey[i];
        if (c.isUpper() && i > 0) {
          snakeCaseKey += '_';
        }
        snakeCaseKey += c.toLower();
      }

      if (started) {
        params_memory.put("TestAlert", snakeCaseKey.remove('_').replace(0, 1, snakeCaseKey[0].toLower()).toStdString());
      } else {
        std::thread([this, key, snakeCaseKey]() {
          playSound(snakeCaseKey.toStdString(), params.getInt(key.toStdString()));
        }).detach();
      }
    });
  }

  QObject::connect(parent, &FrogPilotSettingsWindow::closeParentToggle, [soundsLayout, soundsPanel] {soundsLayout->setCurrentWidget(soundsPanel);});
  QObject::connect(uiState(), &UIState::uiUpdate, this, &FrogPilotSoundsPanel::updateState);
}

void FrogPilotSoundsPanel::showEvent(QShowEvent *event) {
  frogpilotToggleLevels = parent->frogpilotToggleLevels;
  hasBSM = parent->hasBSM;
  hasOpenpilotLongitudinal = parent->hasOpenpilotLongitudinal;
  tuningLevel = parent->tuningLevel;
}

void FrogPilotSoundsPanel::updateState(const UIState &s) {
  if (!isVisible()) {
    return;
  }

  started = s.scene.started;
}

void FrogPilotSoundsPanel::updateToggles() {
  for (auto &[key, toggle] : toggles) {
    if (parentKeys.find(key) != parentKeys.end()) {
      toggle->setVisible(false);
    }
  }

  for (auto &[key, toggle] : toggles) {
    if (parentKeys.find(key) != parentKeys.end()) {
      continue;
    }

    bool setVisible = parent->tuningLevel >= parent->frogpilotToggleLevels[key].toDouble();

    if (key == "LoudBlindspotAlert") {
      setVisible &= hasBSM;
    }

    if (key == "SpeedLimitChangedAlert") {
      setVisible &= params.getBool("ShowSpeedLimits") || (hasOpenpilotLongitudinal && params.getBool("SpeedLimitController"));
    }

    toggle->setVisible(setVisible);

    if (setVisible) {
      if (alertVolumeControlKeys.find(key) != alertVolumeControlKeys.end()) {
        toggles["AlertVolumeControl"]->setVisible(true);
      } else if (customAlertsKeys.find(key) != customAlertsKeys.end()) {
        toggles["CustomAlerts"]->setVisible(true);
      }
    }
  }

  update();
}
