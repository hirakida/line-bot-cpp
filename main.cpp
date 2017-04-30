#include <cppcms/application.h>
#include <cppcms/applications_pool.h>
#include <cppcms/service.h>
#include <cppcms/http_request.h>
#include <cppcms/http_response.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include "line_bot_client.h"


class app : public cppcms::application {
public:
    app(cppcms::service &srv) : cppcms::application(srv) {
        dispatcher().assign("/callback", &app::callback, this);
        mapper().assign("callback", "/callback");
        mapper().root("/");
    }

    void callback();
};

void app::callback() {
    if (request().request_method() == "POST" && is_json(request().content_type())) {
        auto events = line_bot_client::load_events(request().raw_post_data());
        for (auto event : events) {
            line_bot_client::reply_text(event.replyToken, event.message.text);
            line_bot_client::push_text(event.source.userId, "push message");
        }
    } else {
        response().make_error_response(404);
    }
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
