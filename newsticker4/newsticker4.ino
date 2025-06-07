#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>

// WiFi credentials
const char* ssid = "Your SSD";
const char* password = "Your WIFI Password";

// RSS Feed URLs - DEFINE YOUR RSS FEEDS HERE"
const char* newsUrls[] = {
   "https://www.nu.nl/rss/Algemeen",
   "https://feeds.bbci.co.uk/news/rss.xml",
   //"feeds.nos.nl/nosnieuwsalgemeen",
   "https://feeds.skynews.com/feeds/rss/home.xml",
   "https://www.reutersagency.com/feed/?best-types=reuters-news-first&post_type=best"
};
const int numFeeds = 4; // DEFINE THE NUMBER OF LIVE RSS FEEDS
int currentFeed = 0; // START RSS URL

// SH1106 Display
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

String headlines[40];
int numHeadlines = 0;
int xOffset = 128; // Start position off the right side of the display
int speed = 2.5;     // Scrolling speed

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected!");
    
    u8g2.begin();
    u8g2.setFont(u8g2_font_courR14_tf);  // Use font with Latin-1 support
    fetchHeadlines();
}

void loop() {
    u8g2.clearBuffer();
    
    // Draw all headlines horizontally
    int currentX = xOffset;
    for (int i = 0; i < numHeadlines; i++) {
        u8g2.drawUTF8(currentX, 40, headlines[i].c_str());
        currentX += u8g2.getUTF8Width(headlines[i].c_str()) + 10;
    }

    u8g2.sendBuffer();
    
    xOffset -= speed;
    if (xOffset < -getTotalWidth() - 10) {  // Check if all headlines have scrolled off
        xOffset = 128; // Reset position
        currentFeed = (currentFeed + 1) % numFeeds; // Switch feed
        fetchHeadlines();
    }
    
    delay(1);
}

void fetchHeadlines() {
    WiFiClientSecure client;
    client.setInsecure(); // Ignore SSL certificate validation
    HTTPClient http;
    
    if (http.begin(client, newsUrls[currentFeed])) {
        int httpCode = http.GET();
        if (httpCode > 0) {
            String payload = http.getString();
            parseRSS(payload);
        }
        http.end();
    }
}

void parseRSS(String xml) {
    numHeadlines = 0;
    int pos = 0;
    while ((pos = xml.indexOf("<title>", pos)) >= 0 && numHeadlines < 40) {
        int endPos = xml.indexOf("</title>", pos);
        if (endPos > pos) {
            String headline = xml.substring(pos + 7, endPos);
            
            // Remove CDATA tags
            headline.replace("<![CDATA[", "");
            headline.replace("]]>", "");

            // Ensure UTF-8 encoding (needed for U8g2)
            headline = convertToUTF8(headline);

            if (numHeadlines > 0) {
                headlines[numHeadlines] = "-|- " + headline;
            } else {
                headlines[numHeadlines] = headline;
            }
            
            numHeadlines++;
        }
        pos = endPos + 8;
    }
}

// Function to ensure proper UTF-8 encoding
String convertToUTF8(String str) {
    str.replace("£", "\xC2\xA3"); // Pound sign
    str.replace("é", "\xC3\xA9"); // é
    str.replace("ë", "\xC3\xAB"); // ë
    str.replace("ï", "\xC3\xAF"); // ï
    return str;
}

// Function to calculate the total width of all headlines combined
int getTotalWidth() {
    int totalWidth = 0;
    for (int i = 0; i < numHeadlines; i++) {
        totalWidth += u8g2.getUTF8Width(headlines[i].c_str()) + 10;
    }
    return totalWidth;
}
