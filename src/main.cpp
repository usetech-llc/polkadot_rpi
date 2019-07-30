#include <gtk/gtk.h>
#include <iostream>
#include <polkacpp/polkacpp.h>
#include <thread>

#include "private_key.h"

using namespace std;

string addressFrom = "5ECcjykmdAQK71qHBCkEWpWkoMJY6NXvpdKy8UeMx16q5gFr";
string addressTo = "5FpxCaAovn3t2sTsbBeT5pWTj2rg392E8QoduwAyENcPrKht";
int buttonTextFontSize = 70;
int balanceFontSize = 60;
int addressFontSize = 50;
int statusFontSize = 30;
int addressLabelCutoff = 15;

GtkWidget *MainWindow;
GtkWidget *background;
GtkWidget *fixedPanel;
GtkWidget *button;
GtkWidget *addressLabelFrom;
GtkWidget *addressLabelTo;
GtkWidget *balanceLabel;
GtkWidget *progressLabel;

thread *workerThread = nullptr;
IApplication *api;

void UpdateBalance(double balance) {
    char balanceStr[1024];

    if (balance >= 1)
        sprintf(balanceStr, "<span font='%d' color='#00FF00'><b>Balance:</b> %.3f DOT</span>", balanceFontSize,
                balance);
    else
        sprintf(balanceStr, "<span font='%d' color='#00FF00'><b>Balance:</b> %.3f mDOT</span>", balanceFontSize,
                balance * 1000);
    gtk_label_set_markup(GTK_LABEL(balanceLabel), balanceStr);
    gtk_widget_show(balanceLabel);
}

void UpdateProgress(string msg) {
    char progressStr[1024];
    sprintf(progressStr, "<span font='%d' color='#CCCCCC'>Status: %s</span>", statusFontSize, msg.c_str());
    gtk_label_set_markup(GTK_LABEL(progressLabel), progressStr);
    gtk_widget_show(progressLabel);
}

void SubscribeBalance() {
    // Load balance from Polkadot API
    api->subscribeBalance(addressFrom, [&](uint128 balance) {
        balance /= 1000000000000;
        long balLong = (long)balance;

        // Show balance in the UI
        UpdateBalance((double)balLong / 1000.);
    });
}

void SendDotsThread() {
    UpdateProgress("Transferring DOTs - Sending transaction...");
    api->signAndSendTransfer(addressFrom, senderPrivateKeyStr, addressTo, 1000000000000, [&](string result) {
        if (result == "ready")
            UpdateProgress("Transferring DOTs - Registered in Network...");
        if (result == "finalized") {
            UpdateProgress("Transferring DOTs - Transaction Mined!");
            usleep(5000000);
            UpdateProgress("Ready");
        }
    });
}

gboolean button_click_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (workerThread) {
        workerThread->join();
        delete workerThread;
    }

    workerThread = new thread(SendDotsThread);
    return FALSE; // Return false so event will be called again
}

void CreateButton() {
    button = gtk_button_new_with_label("");
    GList *list = gtk_container_get_children(GTK_CONTAINER(button));

    char btnText[128];
    sprintf(btnText, "<span font='%d'>Send 1 mDOT</span>", buttonTextFontSize);
    gtk_label_set_markup(GTK_LABEL(list->data), btnText);

    g_signal_connect(button, "clicked", G_CALLBACK(button_click_event), (gpointer)fixedPanel);

    /* This packs the button into the fixed containers window. */
    gtk_fixed_put(GTK_FIXED(fixedPanel), button, 50, 550);

    /* The final step is to display this newly created widget. */
    gtk_widget_show(button);
}

void CreateLabels() {
    addressLabelFrom = gtk_label_new(NULL);
    addressLabelTo = gtk_label_new(NULL);
    balanceLabel = gtk_label_new(NULL);
    progressLabel = gtk_label_new(NULL);

    char addrLbl[1024];
    sprintf(addrLbl, "<span font='%d' color='#CCCCCC'><b>Wallet:</b> %s...</span>", addressFontSize,
            addressFrom.substr(0, addressLabelCutoff).c_str());
    gtk_label_set_markup(GTK_LABEL(addressLabelFrom), addrLbl);

    sprintf(addrLbl, "<span font='%d' color='#CCCCCC'><b>Send to:</b> %s...</span>", addressFontSize,
            addressTo.substr(0, addressLabelCutoff).c_str());
    gtk_label_set_markup(GTK_LABEL(addressLabelTo), addrLbl);
    gtk_label_set_markup(GTK_LABEL(balanceLabel), "");
    gtk_label_set_markup(GTK_LABEL(progressLabel), "");

    gtk_fixed_put(GTK_FIXED(fixedPanel), addressLabelFrom, 50, 170);
    gtk_fixed_put(GTK_FIXED(fixedPanel), addressLabelTo, 50, 270);
    gtk_fixed_put(GTK_FIXED(fixedPanel), balanceLabel, 50, 370);
    gtk_fixed_put(GTK_FIXED(fixedPanel), progressLabel, 50, 500);

    gtk_widget_show(addressLabelFrom);
    gtk_widget_show(addressLabelTo);
    gtk_widget_show(balanceLabel);
    gtk_widget_show(progressLabel);
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

    //----- ENTER THE GTK MAIN LOOP -----
    gtk_main(); // Enter the GTK+ main loop until the application closes.

    if (workerThread) {
        workerThread->join();
        delete workerThread;
    }

    api->unsubscribeBalance(addressFrom);
    api->disconnect();

    return 0;
}
