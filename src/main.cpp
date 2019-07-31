#include <clocale>
#include <gtk/gtk.h>
#include <iostream>
#include <mutex>
#include <polkacpp/polkacpp.h>
#include <thread>

#include "private_key.h"

using namespace std;

string addressFrom = "5ECcjykmdAQK71qHBCkEWpWkoMJY6NXvpdKy8UeMx16q5gFr";
string addressTo = "5FpxCaAovn3t2sTsbBeT5pWTj2rg392E8QoduwAyENcPrKht";

// UI Parameters
int statusLabelX = 380;
int statusLabelY = 710;

int addressFromX = 100;
int addressFromY = 350;
int addressToX = 690;
int addressToY = 350;

int balance1X = 70;
int balance1Y = 400;
int balance2X = 660;
int balance2Y = 400;

int progressBarLabelX = 385;
int progressBarLabelY = 190;
int progressDots = 8;
bool inProgress = false;

int buttonTextFontSize = 70;
int balanceFontSize = 40;
int addressFontSize = 30;
int statusFontSize = 30;
int addressLabelCutoff = 7;

GtkWidget *MainWindow;
GtkWidget *background;
GtkWidget *fixedPanel;
GtkWidget *button;
GtkWidget *addressLabelFrom;
GtkWidget *addressLabelTo;
GtkWidget *balanceLabel1;
GtkWidget *balanceLabel2;
GtkWidget *progressLabel;
GtkWidget *progressBarLabel;
GtkWidget *image;

thread *workerThread = nullptr;
thread *updateProgressThread = nullptr;
IApplication *api;

mutex uilock;

void UpdateBalance(GtkWidget *label, double balance) {
    char balanceStr[1024];

    std::setlocale(LC_ALL, "en_US.UTF-8");
    std::setlocale(LC_NUMERIC, "de_DE.UTF-8");

    if (balance >= 1)
        sprintf(balanceStr, "<span font='%d' font_family='MullerBold' color='#00FF00'>%.3f DOT</span>", balanceFontSize,
                balance);
    else
        sprintf(balanceStr, "<span font='%d' font_family='MullerBold' color='#00FF00'>%.1f mDOT</span>",
                balanceFontSize, balance * 1000);

    uilock.lock();
    gtk_label_set_markup(GTK_LABEL(label), balanceStr);
    gtk_widget_show(label);
    uilock.unlock();
}

void UpdateProgress(string msg) {
    char progressStr[1024];
    sprintf(progressStr,
            "<span font='%d' font_family='MullerBold' color='#000'>Status:</span><span font='%d' "
            "font_family='MullerRegular' color='#000'> %s</span>",
            statusFontSize, statusFontSize, msg.c_str());

    uilock.lock();
    gtk_label_set_markup(GTK_LABEL(progressLabel), progressStr);
    gtk_widget_show(progressLabel);
    uilock.unlock();
}

void UpdateProgressBar(int greenDots, int totalDots) {
    char progressStr[1024];
    char greenStr[100] = {0};
    char greyStr[100] = {0};

    int i = 0;
    for (; i < greenDots; ++i) {
        greenStr[i] = '.';
    }
    int j = 0;
    for (; i < totalDots; ++i) {
        greyStr[j++] = '.';
    }

    sprintf(progressStr,
            "<span font='95' font_family='MullerBold' color='#27d222'>%s</span>"
            "<span font='95' font_family='MullerBold' color='#d8d8d8'>%s</span>",
            greenStr, greyStr);

    uilock.lock();
    gtk_label_set_markup(GTK_LABEL(progressBarLabel), progressStr);
    gtk_widget_show(progressBarLabel);
    uilock.unlock();
}

void UpdateProgressBarThread() {
    int greenDots = 0;

    while (inProgress) {
        UpdateProgressBar(greenDots, progressDots);
        greenDots = (greenDots + 1) % (progressDots + 1);
        usleep(100000);
    }
    UpdateProgressBar(0, progressDots);
}

void SubscribeBalance() {
    // Load balance from Polkadot API
    api->subscribeBalance(addressFrom, [&](uint128 balance) {
        balance /= 1000000000000;
        long balLong = (long)balance;

        // Show balance in the UI
        UpdateBalance(balanceLabel1, (double)balLong / 1000.);
    });

    api->subscribeBalance(addressTo, [&](uint128 balance) {
        balance /= 1000000000000;
        long balLong = (long)balance;

        // Show balance in the UI
        UpdateBalance(balanceLabel2, (double)balLong / 1000.);
    });
}

//#define EMULATE

void SendDotsThread() {
    UpdateProgress("Sending transaction...");
#ifndef EMULATE
    api->signAndSendTransfer(addressFrom, senderPrivateKeyStr, addressTo, 1000000000000, [&](string result) {
        if (result == "ready")
            UpdateProgress("Registered in Network...");
        if (result == "finalized") {
            UpdateProgress("Transaction Mined!");
            inProgress = false;
            usleep(5000000);
            UpdateProgress("Ready");
        }
    });
#else
    usleep(1000000);
    UpdateProgress("Registered in Network...");
    usleep(1000000);
    UpdateProgress("Transaction Mined!");
    inProgress = false;
    usleep(1000000);
    UpdateProgress("Ready");
#endif
}

gboolean button_click_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (workerThread) {
        workerThread->join();
        delete workerThread;
        workerThread = nullptr;
    }
    if (updateProgressThread) {
        updateProgressThread->join();
        delete updateProgressThread;
        updateProgressThread = nullptr;
    }

    inProgress = true;
    workerThread = new thread(SendDotsThread);
    updateProgressThread = new thread(UpdateProgressBarThread);

    return FALSE; // Return false so event will be called again
}

void CreateButton() {
    button = gtk_button_new();
    image = gtk_image_new_from_file("button.png");
    gtk_button_set_image(GTK_BUTTON(button), image);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    // gtk_button_set_always_show_image(GTK_BUTTON(button), true);

    // Style the button
    GtkStyleContext *context = gtk_widget_get_style_context(button);
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(provider),
                                    " GtkButton {\n"
                                    "   -GtkWidget-focus-line-width: 0;\n"
                                    "   border: 0;\n"
                                    "}\n"
                                    " .button:hover {\n"
                                    "   opacity: 0.00;\n"
                                    "}\n"
                                    " .button:active {\n"
                                    "   opacity: 0.00;\n"
                                    "}\n",
                                    -1, NULL);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_signal_connect(button, "clicked", G_CALLBACK(button_click_event), (gpointer)fixedPanel);

    /* This packs the button into the fixed containers window. */
    gtk_fixed_put(GTK_FIXED(fixedPanel), button, 315, 544);

    /* The final step is to display this newly created widget. */
    gtk_widget_show(button);
}

void CreateLabels() {
    addressLabelFrom = gtk_label_new(NULL);
    addressLabelTo = gtk_label_new(NULL);
    balanceLabel1 = gtk_label_new(NULL);
    balanceLabel2 = gtk_label_new(NULL);
    progressLabel = gtk_label_new(NULL);
    progressBarLabel = gtk_label_new(NULL);

    char addrLbl[1024];
    sprintf(addrLbl, "<span font='%d' font_family='MullerBold' color='#000'>%s...</span>", addressFontSize,
            addressFrom.substr(0, addressLabelCutoff).c_str());
    gtk_label_set_markup(GTK_LABEL(addressLabelFrom), addrLbl);

    sprintf(addrLbl, "<span font='%d' font_family='MullerBold' color='#000'>%s...</span>", addressFontSize,
            addressTo.substr(0, addressLabelCutoff).c_str());
    gtk_label_set_markup(GTK_LABEL(addressLabelTo), addrLbl);
    gtk_label_set_markup(GTK_LABEL(balanceLabel1), "");
    gtk_label_set_markup(GTK_LABEL(balanceLabel2), "");
    gtk_label_set_markup(GTK_LABEL(progressLabel), "");
    gtk_label_set_markup(GTK_LABEL(progressBarLabel), "");

    gtk_fixed_put(GTK_FIXED(fixedPanel), addressLabelFrom, addressFromX, addressFromY);
    gtk_fixed_put(GTK_FIXED(fixedPanel), addressLabelTo, addressToX, addressToY);
    gtk_fixed_put(GTK_FIXED(fixedPanel), balanceLabel1, balance1X, balance1Y);
    gtk_fixed_put(GTK_FIXED(fixedPanel), balanceLabel2, balance2X, balance2Y);
    gtk_fixed_put(GTK_FIXED(fixedPanel), progressLabel, statusLabelX, statusLabelY);
    gtk_fixed_put(GTK_FIXED(fixedPanel), progressBarLabel, progressBarLabelX, progressBarLabelY);

    gtk_widget_show(addressLabelFrom);
    gtk_widget_show(addressLabelTo);
    gtk_widget_show(balanceLabel1);
    gtk_widget_show(balanceLabel2);
    gtk_widget_show(progressLabel);
    gtk_widget_show(progressBarLabel);
}

int main(int argc, char *argv[]) {

    //---------------------------------
    //----- CREATE THE GTK WINDOW -----
    //---------------------------------

    gtk_init(&argc, &argv);

    MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(MainWindow), false);
    // gtk_window_set_title(GTK_WINDOW(MainWindow), "Polkadot GUI for C++ API");
    gtk_container_set_border_width(GTK_CONTAINER(MainWindow), 1);
    gtk_widget_set_size_request(GTK_WIDGET(MainWindow), 1024, 768);
    gtk_window_set_position(GTK_WINDOW(MainWindow), GTK_WIN_POS_CENTER);

    /* Create a Fixed Container */
    fixedPanel = gtk_fixed_new();
    gtk_container_add(GTK_CONTAINER(MainWindow), fixedPanel);
    gtk_widget_show(fixedPanel);

    // background
    background = gtk_image_new_from_file("background.png");
    gtk_fixed_put(GTK_FIXED(fixedPanel), background, 0, 0);

    CreateButton();
    CreateLabels();

    gtk_widget_show_all(MainWindow);

    // Close the application if the x button is pressed if ALT+F4 is used
    g_signal_connect(G_OBJECT(MainWindow), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Connect API
    api = polkadot::api::getInstance()->app();
    api->connect("");

    SubscribeBalance();
    UpdateProgress("Ready");
    UpdateProgressBar(0, progressDots);

    //----- ENTER THE GTK MAIN LOOP -----
    gtk_main(); // Enter the GTK+ main loop until the application closes.

    if (workerThread) {
        workerThread->join();
        delete workerThread;
    }
    if (updateProgressThread) {
        updateProgressThread->join();
        delete updateProgressThread;
    }

    api->unsubscribeBalance(addressFrom);
    api->disconnect();

    return 0;
}
