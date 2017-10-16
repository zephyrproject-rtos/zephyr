/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#ifndef _HTTP_PARSER_STATE_H_
#define _HTTP_PARSER_STATE_H_
#ifdef __cplusplus
extern "C" {
#endif

enum state {
	s_dead = 1, /* important that this is > 0 */
	s_start_req_or_res,
	s_res_or_resp_H,
	s_start_res,
	s_res_H,
	s_res_HT,
	s_res_HTT,
	s_res_HTTP,
	s_res_first_http_major,
	s_res_http_major,
	s_res_first_http_minor,
	s_res_http_minor,
	s_res_first_status_code,
	s_res_status_code,
	s_res_status_start,
	s_res_status,
	s_res_line_almost_done,
	s_start_req,
	s_req_method,
	s_req_spaces_before_url,
	s_req_schema,
	s_req_schema_slash,
	s_req_schema_slash_slash,
	s_req_server_start,
	s_req_server,
	s_req_server_with_at,
	s_req_path,
	s_req_query_string_start,
	s_req_query_string,
	s_req_fragment_start,
	s_req_fragment,
	s_req_http_start,
	s_req_http_H,
	s_req_http_HT,
	s_req_http_HTT,
	s_req_http_HTTP,
	s_req_first_http_major,
	s_req_http_major,
	s_req_first_http_minor,
	s_req_http_minor,
	s_req_line_almost_done,
	s_header_field_start,
	s_header_field,
	s_header_value_discard_ws,
	s_header_value_discard_ws_almost_done,
	s_header_value_discard_lws,
	s_header_value_start,
	s_header_value,
	s_header_value_lws,
	s_header_almost_done,
	s_chunk_size_start,
	s_chunk_size,
	s_chunk_parameters,
	s_chunk_size_almost_done,
	s_headers_almost_done,
	s_headers_done,
	/* Important: 's_headers_done' must be the last 'header' state. All
	 * states beyond this must be 'body' states. It is used for overflow
	 * checking. See the PARSING_HEADER() macro.
	 */
	s_chunk_data,
	s_chunk_data_almost_done,
	s_chunk_data_done,
	s_body_identity,
	s_body_identity_eof,
	s_message_done
};

#ifdef __cplusplus
}
#endif
#endif
