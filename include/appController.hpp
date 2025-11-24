#ifndef APP_CONTROLLER_HPP
#define APP_CONTROLLER_HPP

#include "assessmentSession.hpp"
#include "displayManager.hpp"
#include "exportSession.hpp"
#include "geminiManager.hpp"
#include "inputHandler.hpp"
#include "questionGenerator.hpp"
#include "questionManager.hpp" 
#include "storageManager.hpp"
#include "webServerManager.hpp"
#include "data.hpp"

enum AppState { STATE_MENU, STATE_TEST, STATE_EXPORT, STATE_GENERATE };

class AppController {
  public:
    AppController();

    void begin();
    void loop();

  private:
    void enterMenu();
    void handleMenu();
    void startTest();
    void handleTest();
    void enterExport();
    void handleExport();
    void enterGenerate();
    void handleGenerate();
    void resetToMenu();
    void useDefaultQuestions();

    QuestionManager questionManager_;
    InputHandler inputHandler_;
    DisplayManager displayManager_;
    WebServerManager webServerManager_;
    GeminiManager geminiManager_;
    AssessmentSession assessmentSession_;
    ExportSession exportSession_;
    QuestionGenerator questionGenerator_;

    AssessmentState current_status_{DEFAULT_QUESTIONS};
    AppState state_{STATE_MENU};
    int menuIndex_{0};
};

#endif
