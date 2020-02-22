// SPDX-License-Identifier: MIT

#include "stream.h"
#include <cctype>
#include <cstring>
#include <thread>

namespace plac {

namespace {

FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes,
                                            void *client_data) {
  Stream *dec = static_cast<Stream *>(client_data);

  if (*bytes > 0) {
    ssize_t r = read(dec->desc_.fd_, buffer, *bytes);

    if (r > 0) {
      *bytes = r;
      return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    } else if (r == 0) {
      return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    } else {
      return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    }
  } else {
    return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
  }
}

FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame,
                                              const FLAC__int32 *const buffer[], void *client_data) {
  const Stream *stream = static_cast<Stream *>(client_data);

  if ((frame->header.bits_per_sample != 16 && frame->header.bits_per_sample != 24) || frame->header.channels != 2) {
    LOG_ERROR("FLAC format not supported");
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
  }

  if (frame->header.bits_per_sample != stream->format_.bits) {
    LOG_ERROR("mismatch of bits per sample");
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
  }
  if (frame->header.sample_rate != stream->format_.rate) {
    LOG_ERROR("mismatch of sample rate");
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
  }
  if (frame->header.channels != stream->format_.channels) {
    LOG_ERROR("mismatch of number of channels");
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
  }

  size_t length = frame->header.blocksize;
  const int *left = buffer[0];
  const int *right = buffer[1];
  while (length != 0) {
    const ssize_t n = stream->audio_buffer_.Write(stream->format_, left, right, length);
    if ((n >= 0) && (static_cast<size_t>(n) < length)) {
      std::this_thread::sleep_for(std::chrono::milliseconds{5});
    }
    length -= n;
    left += n;
    right += n;
  }

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
  LOG_ERROR("FLAC error callback");
}

static int tagcompare(const char *s1, const char *s2, int n) {
  int c = 0;
  while (c < n) {
    if (toupper(s1[c]) != toupper(s2[c]))
      return !0;
    c++;
  }
  return 0;
}

const char *vorbis_comment_query(const FLAC__StreamMetadata_VorbisComment &vc, const char *tag, int count) {
  int found = 0;
  int taglen = strlen(tag) + 1; /* +1 for the = we append */
  char *fulltag = (char *)alloca(taglen + 1);

  strcpy(fulltag, tag);
  strcat(fulltag, "=");

  for (long i = 0; i < vc.num_comments; i++) {
    if (!tagcompare((const char *)vc.comments[i].entry, fulltag, taglen)) {
      if (count == found)
        /* We return a pointer to the data, not a copy */
        return (const char *)vc.comments[i].entry + taglen;
      else {
        found++;
      }
    }
  }
  return "<none>"; /* didn't find anything */
}

void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
  if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
    Stream *stream = static_cast<Stream *>(client_data);
    stream->format_.rate = metadata->data.stream_info.sample_rate;
    stream->format_.bits = metadata->data.stream_info.bits_per_sample;
    stream->format_.channels = metadata->data.stream_info.channels;
  } else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
    printf("%s/%s: %s - %s | %s\n", vorbis_comment_query(metadata->data.vorbis_comment, "TRACKNUMBER", 0),
           vorbis_comment_query(metadata->data.vorbis_comment, "TRACKTOTAL", 0),
           vorbis_comment_query(metadata->data.vorbis_comment, "TITLE", 0),
           vorbis_comment_query(metadata->data.vorbis_comment, "ARTIST", 0),
           vorbis_comment_query(metadata->data.vorbis_comment, "ALBUM", 0));
    fflush(stdout);
  }
}

} // namespace

Stream::Stream(AudioBuffer<228000> &audio_buffer)
    : decoder_{FLAC__stream_decoder_new()}, desc_{}, format_{}, audio_buffer_{audio_buffer} {
  FLAC__stream_decoder_set_metadata_respond(decoder_, FLAC__METADATA_TYPE_VORBIS_COMMENT);
  FLAC__StreamDecoderInitStatus init_status =
      FLAC__stream_decoder_init_stream(decoder_, read_callback, nullptr, nullptr, nullptr, nullptr, write_callback,
                                       metadata_callback, error_callback, this);
  ENSURES(init_status == FLAC__STREAM_DECODER_INIT_STATUS_OK, "cannot initialize FLAC decoder");
}

void Stream::Decode() {
  FLAC__bool ret = FLAC__stream_decoder_process_until_end_of_stream(decoder_);
  ENSURES(ret == true, "stream decoding error");
}

void Stream::FillBuffer() {
  FLAC__StreamDecoderState state = FLAC__stream_decoder_get_state(decoder_);
  while (state != FLAC__STREAM_DECODER_END_OF_STREAM && audio_buffer_.GetFillLevel() < 0.8F) {
    FLAC__bool ret = FLAC__stream_decoder_process_single(decoder_);
    ENSURES(ret == true, "stream decoding error");
    state = FLAC__stream_decoder_get_state(decoder_);
  }
}

} // namespace plac
