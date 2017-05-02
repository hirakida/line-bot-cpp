#ifndef line_bot_cppcms_h
#define line_bot_cppcms_h

#include <cppcms/application.h>
#include <cppcms/json.h>
#include <curl/curl.h>
#include "picojson.h"

namespace line_bot {

    static bool is_json(const std::string &content_type) {
        std::string::size_type pos = content_type.find("application/json");
        return pos != std::string::npos;
    }

    static bool is_callback_request(cppcms::http::request &request) {
        // TODO: Signature validation
        return request.request_method() == "POST"
               && line_bot::is_json(request.content_type());
    }

    class model {
    public:
        static picojson::array &parse_events(picojson::value &val, cppcms::http::request &request) {
            std::pair<void *, ssize_t> post = request.raw_post_data();
            std::string err;
            picojson::parse(val, (const char *) post.first, (const char *) post.first + post.second, &err);
            if (!err.empty()) {
                throw std::runtime_error(err.c_str());
            }
            picojson::object &obj = val.get<picojson::object>();
            picojson::array &events = obj["events"].get<picojson::array>();
            return events;
        }

        static std::string replyToken(picojson::object &event) {
            return event["replyToken"].get<std::string>();
        }

        // event type
        static bool is_message_event(picojson::object &event) {
            return event["type"].get<std::string>() == "message";
        }

        // source
        static std::string userId(picojson::object &event) {
            // TODO: group or roomかも判定する
            picojson::object &source = event["source"].get<picojson::object>();
            return source["userId"].get<std::string>();
        }

        // message type
        static bool is_text(picojson::object &event) {
            picojson::object &message = get_message(event);
            return message["type"].get<std::string>() == "text";
        }

        static bool is_sticker(picojson::object &event) {
            picojson::object &message = get_message(event);
            return message["type"].get<std::string>() == "sticker";
        }

        // message
        static std::string text(picojson::object &event) {
            picojson::object &message = get_message(event);
            return message["text"].get<std::string>();
        }

        static std::string packageId(picojson::object &event) {
            picojson::object &message = get_message(event);
            return message["packageId"].get<std::string>();
        }

        static std::string stickerId(picojson::object &event) {
            picojson::object &message = get_message(event);
            return message["stickerId"].get<std::string>();
        }

    private:
        static picojson::object &get_message(picojson::object &event) {
            return event["message"].get<picojson::object>();
        }
    };

    class client {
    public:
        static void reply_text(std::string reply_token, std::string text) {
            cppcms::json::value val;
            val["replyToken"] = reply_token;
            val["messages"][0]["type"] = "text";
            val["messages"][0]["text"] = text;
            send(val.save(), send_reply);
        }

        static void reply_sticker(std::string reply_token, std::string packageId, std::string stickerId) {
            cppcms::json::value val;
            val["replyToken"] = reply_token;
            val["messages"][0]["type"] = "sticker";
            val["messages"][0]["packageId"] = packageId;
            val["messages"][0]["stickerId"] = stickerId;
            send(val.save(), send_reply);
        }

        static void push_text(std::string to, std::string text) {
            cppcms::json::value val;
            val["to"] = to;
            val["messages"][0]["type"] = "text";
            val["messages"][0]["text"] = text;
            send(val.save(), send_push);
        }

        static void push_sticker(std::string to, std::string packageId, std::string stickerId) {
            cppcms::json::value val;
            val["to"] = to;
            val["messages"][0]["type"] = "sticker";
            val["messages"][0]["packageId"] = packageId;
            val["messages"][0]["stickerId"] = stickerId;
            send(val.save(), send_push);
        }

    private:
        enum send_type {
            send_reply,
            send_push
        };

        static void send(std::string val, send_type type) {

            CURL *curl = curl_easy_init();
            if (curl == NULL) {
                std::cerr << "curl_easy_init() failed" << std::endl;
                return;
            }
            std::string access_token = std::getenv("ACCESS_TOKEN");
            std::string header = "Authorization: Bearer " + access_token;

            struct curl_slist *headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, header.c_str());

            std::string url = type == send_reply ? "https://api.line.me/v2/bot/message/reply"
                                                 : "https://api.line.me/v2/bot/message/push";
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, val.c_str());

            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed. " << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
        }
    };
}

#endif //line_bot_cppcms_h
