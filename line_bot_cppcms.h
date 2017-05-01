#ifndef LINE_API_CLIENT_H
#define LINE_API_CLIENT_H

#include <time.h>
#include <cppcms/application.h>
#include <cppcms/json.h>
#include <curl/curl.h>

namespace line_bot {

    enum event_type {
        message,
        follow,
        unfollow,
        join,
        leave,
        post_back,
        beacon
    };

    // source
    struct source_user {
        std::string type;   // user
        std::string userId;
    };

    struct source_group {
        std::string type;   // group
        std::string groupId;
    };

    struct source_room {
        std::string type;   // room
        std::string roomId;
    };

    // message event
    struct text_message {
        std::string id;
        std::string type;   // text, image, video, audio, location sticker
        std::string text;
    };

    struct message_event {
        std::string replyToken;
        std::string type; // message
        time_t timestamp;
        source_user source;
        text_message message;
    };

    struct message_events {
        std::vector<message_event> events;
    };

    static bool is_json(const std::string &content_type) {
        std::string::size_type pos = content_type.find("application/json");
        return pos != std::string::npos;
    }

    static bool is_callback_request(cppcms::http::request &request) {
        return request.request_method() == "POST"
               && line_bot::is_json(request.content_type());
    }

    static bool is_message_event(std::string type) {
        return type == "message";
    }

    // まだmessage_event, text_messageのみ
    static std::vector<message_event> parse_events(cppcms::http::request &request) {
        std::pair<void *, ssize_t> post = request.raw_post_data();
        std::istringstream ss(std::string((char *) post.first, (unsigned long) post.second));
        cppcms::json::value val;
        val.load(ss, true);
        message_events events = val.get_value<message_events>();
        return events.events;
    }

    class client {
    public:
        enum send_type {
            send_reply,
            send_push
        };

        static void reply_text(std::string replyToken, std::string text) {
            cppcms::json::value val;
            val["replyToken"] = replyToken;
            val["messages"][0]["type"] = "text";
            val["messages"][0]["text"] = text;
            send_text(val.save(), send_reply);
        }

        static void reply_sticker(std::string replyToken, std::string packageId, std::string stickerId) {
            cppcms::json::value val;
            val["replyToken"] = replyToken;
            val["messages"][0]["type"] = "sticker";
            val["messages"][0]["packageId"] = packageId;
            val["messages"][0]["stickerId"] = stickerId;
            send_text(val.save(), send_reply);
        }

        static void push_text(std::string to, std::string text) {
            cppcms::json::value val;
            val["to"] = to;
            val["messages"][0]["type"] = "text";
            val["messages"][0]["text"] = text;
            send_text(val.save(), send_push);
        }

        static void push_sticker(std::string to, std::string packageId, std::string stickerId) {
            cppcms::json::value val;
            val["to"] = to;
            val["messages"][0]["type"] = "sticker";
            val["messages"][0]["packageId"] = packageId;
            val["messages"][0]["stickerId"] = stickerId;
            send_text(val.save(), send_push);
        }

    private:
        static void send_text(std::string val, send_type type) {

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

namespace cppcms {
    namespace json {
        template<>
        struct traits<line_bot::source_user> {
            static line_bot::source_user get(cppcms::json::value const &v) {
                if (v.type() != cppcms::json::is_object)
                    throw cppcms::json::bad_value_cast();
                line_bot::source_user p;
                p.type = v.get<std::string>("type");
                p.userId = v.get<std::string>("userId");
                return p;
            }
        };

        template<>
        struct traits<line_bot::text_message> {
            static line_bot::text_message get(cppcms::json::value const &v) {
                if (v.type() != cppcms::json::is_object)
                    throw cppcms::json::bad_value_cast();
                line_bot::text_message p;
                p.id = v.get<std::string>("id");
                p.type = v.get<std::string>("type");
                p.text = v.get<std::string>("text");
                return p;
            }
        };

        template<>
        struct traits<line_bot::message_event> {
            static line_bot::message_event get(cppcms::json::value const &v) {
                if (v.type() != cppcms::json::is_object)
                    throw cppcms::json::bad_value_cast();
                line_bot::message_event p;
                p.replyToken = v.get<std::string>("replyToken");
                p.type = v.get<std::string>("type");
                p.timestamp = v.get<time_t>("timestamp");
                p.source = v.get<line_bot::source_user>("source");
                p.message = v.get<line_bot::text_message>("message");
                return p;
            }
        };

        template<>
        struct traits<line_bot::message_events> {
            static line_bot::message_events get(cppcms::json::value const &v) {
                if (v.type() != cppcms::json::is_object)
                    throw cppcms::json::bad_value_cast();
                line_bot::message_events p;
                p.events = v.get<std::vector<line_bot::message_event> >("events");
                return p;
            }
        };
    }
}

#endif //LINE_API_CLIENT_H
