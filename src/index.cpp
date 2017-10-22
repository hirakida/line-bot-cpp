#include <cppcms/application.h>
#include <cppcms/applications_pool.h>
#include <cppcms/service.h>
#include <cppcms/http_request.h>
#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include "line_bot_cppcms.h"

class app : public cppcms::application {

public:
    app(cppcms::service &srv) : cppcms::application(srv) {
        dispatcher().assign("/callback", &app::callback, this);
        mapper().assign("callback", "/callback");
        mapper().root("/");
    }

private:
    void callback();

    void handle_message_event(cppcms::json::value &event);

    void handle_text_message_event(cppcms::json::value &event);

    void handle_sticker_message_event(cppcms::json::value &event);
};

void app::callback() {
    if (!line_bot::is_callback_request(request())) {
        response().make_error_response(404);
    }
    cppcms::json::array events = line_bot::parse_events(request());
    for (cppcms::json::value &event : events) {
        line_bot::event::event_type type = line_bot::event::type(event);
        switch (type) {
            case line_bot::event::event_type::event_message:
                handle_message_event(event);
                break;
            default:
                std::cout << "unsupported event_type: " << type << std::endl;
                break;
        }
    }
}

void app::handle_message_event(cppcms::json::value &event) {
    line_bot::event::message::message_type type = line_bot::event::message::type(event);
    switch (type) {
        case line_bot::event::message::message_text:
            handle_text_message_event(event);
            break;
        case line_bot::event::message::message_sticker:
            handle_sticker_message_event(event);
            break;
        default:
            std::cout << "unsupported message_type: " << type << std::endl;
            break;
    }
}

void app::handle_text_message_event(cppcms::json::value &event) {
    std::string text = line_bot::event::message::text(event);
    std::string token = line_bot::event::reply_token(event);
    if (text == "profile") {
        std::string profile;
        line_bot::client::get_profile(line_bot::event::source::user_id(event), profile);
        line_bot::client::reply_text(token, profile);
    } else if (text == "confirm") {
        line_bot::client::reply_confirm_template(token);
    } else {
        line_bot::client::reply_text(token, text);
    }
}

void app::handle_sticker_message_event(cppcms::json::value &event) {
    line_bot::client::reply_sticker(line_bot::event::reply_token(event),
                                    line_bot::event::message::package_id(event),
                                    line_bot::event::message::sticker_id(event));
}

int main(int argc, char **argv) {
    try {
        cppcms::service srv(argc, argv);
        srv.applications_pool()
                .mount(cppcms::applications_factory<app>());
        srv.run();
    }
    catch (std::exception const &e) {
        std::cerr << e.what() << std::endl;
    }
}
