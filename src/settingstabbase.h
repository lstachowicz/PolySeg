#ifndef SETTINGSTABBASE_H
#define SETTINGSTABBASE_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QFrame>

#include "projectconfig.h"

/**
 * @brief Base class for all settings tab implementations
 *
 * This class implements the Template Method pattern to provide a consistent
 * structure for all settings tabs. It handles the common scroll area setup
 * and defines the contract that all tabs must follow.
 */
class BaseSettingsTab : public QWidget
{
  Q_OBJECT

 public:
  explicit BaseSettingsTab(QWidget* parent = nullptr);
  virtual ~BaseSettingsTab() = default;

  /**
   * @brief Load settings from configuration
   * @param config The project configuration to load from
   */
  virtual void LoadFromConfig(const ProjectConfig& config) = 0;

  /**
   * @brief Save settings to configuration
   * @param config The project configuration to save to
   */
  virtual void SaveToConfig(ProjectConfig& config) = 0;

 protected:
};

#endif  // SETTINGSTABBASE_H
