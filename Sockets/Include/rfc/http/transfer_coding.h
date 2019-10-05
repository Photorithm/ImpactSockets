/**
 * Created by TekuConcept on October 4, 2019
 */

#ifndef _IMPACT_HTTP_TRANSFER_CODING_H_
#define _IMPACT_HTTP_TRANSFER_CODING_H_

#include <iostream>
#include <string>
#include <vector>
#include <set>

#include "utils/case_string.h"
#include "rfc/http/header_list.h"

namespace impact {
namespace http {

    class transfer_coding {
    public:
        transfer_coding(std::string name);
        virtual ~transfer_coding();

        inline const case_string& name() const
        { return m_name_; }

        virtual std::string encode(const std::string& buffer) = 0;

    private:
        case_string m_name_;

        transfer_coding();
        bool _M_valid_transfer_extension(const std::string&) const;

        friend class chunked_coding;
    };


    class chunked_observer;
    class chunked_coding final : public transfer_coding {
    public:
        chunked_coding();
        ~chunked_coding();

        inline void register_observer(chunked_observer* observer)
        { m_observer_ = observer; }

        std::string encode(const std::string& buffer);

        struct extension_t {
            extension_t(
                std::string name,
                std::string value = "");
            ~extension_t();
            inline const std::string& name() const { return m_name_; }
            inline const std::string& value() const { return m_value_; }
            void name(std::string name);
            void value(std::string value);
            std::string to_string() const;
            static extension_t parse(std::string);
         private:
            std::string m_name_;
            std::string m_value_;
            extension_t();
            static bool _M_parse(const std::string&,extension_t*);
            friend std::ostream& operator<<(std::ostream&, const extension_t&);
        };

    private:
        chunked_observer* m_observer_;

        static std::set<case_string> s_forbidden_trailers_;
    };


    class chunked_observer {
    public:
        virtual ~chunked_observer() = default;
        virtual void on_next_chunk(
            std::vector<chunked_coding::extension_t>& extensions,
            const std::string& buffer) = 0;
        virtual void on_last_chunk(
            std::vector<chunked_coding::extension_t>& extensions,
            header_list& trailers) = 0;
    };

}}

#endif
