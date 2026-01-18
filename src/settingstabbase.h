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
   * @brief Template Method - defines the skeleton algorithm for tab initialization
   *
   * This method ensures all tabs follow the same initialization sequence:
   * 1. Setup scroll area (common implementation)
   * 2. Setup UI elements (tab-specific)
   * 3. Connect signals (tab-specific)
   */
  void Initialize()
  {
    SetupScrollArea();   // Common implementation (final)
    SetupUI();           // Tab-specific (virtual)
    ConnectSignals();    // Tab-specific (virtual)
  }

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
  /**
   * @brief Hook method for setting up tab-specific UI elements
   *
   * Subclasses must implement this to create their specific UI.
   * Use GetContentWidget() and GetMainLayout() to add elements.
   */
  virtual void SetupUI() = 0;

  /**
   * @brief Hook method for connecting tab-specific signals
   *
   * Subclasses must implement this to connect their signal/slot handlers.
   */
  virtual void ConnectSignals() = 0;

  /**
   * @brief Get the content widget for adding UI elements
   * @return Pointer to the scrollable content widget
   */
  QWidget* GetContentWidget() const { return content_widget_; }

  /**
   * @brief Get the main layout for adding UI elements
   * @return Pointer to the main vertical layout
   */
  QVBoxLayout* GetMainLayout() const { return main_layout_; }

 private:
  /**
   * @brief Setup the scroll area (final implementation)
   *
   * This method is called by Initialize() and creates a consistent
   * scroll area structure for all tabs. It cannot be overridden.
   */
  void SetupScrollArea();

  QScrollArea* scroll_area_;
  QWidget* content_widget_;
  QVBoxLayout* main_layout_;
};

#endif  // SETTINGSTABBASE_H
