MODULE = CryptX         PACKAGE = Crypt::AuthEnc::EAX

Crypt::AuthEnc::EAX
_new(char * cipher_name, SV * key, SV * nonce, SV * header=&PL_sv_undef)
    CODE:
    {
        STRLEN k_len=0;
        unsigned char *k=NULL;
        unsigned char *n=NULL;
        STRLEN n_len=0;
        unsigned char *h=NULL;
        STRLEN h_len=0;
        int id;

        if (!SvPOK(key))   croak("FATAL: key must be string/buffer scalar");
        k = (unsigned char *) SvPVbyte(key, k_len);
        if (!SvPOK(nonce)) croak("FATAL: nonce must be string/buffer scalar");
        n = (unsigned char *) SvPVbyte(nonce, n_len);
        if(SvOK(header)) { /* header is optional param */
          if(!SvPOK(header)) croak("FATAL: header must be string/buffer scalar");
          h = (unsigned char *) SvPVbyte(header, h_len);
        }

        id = find_cipher(cipher_name);
        if(id==-1) croak("FATAL: find_cipfer failed for '%s'", cipher_name);

        Newz(0, RETVAL, 1, struct eax_struct);
        if (!RETVAL) croak("FATAL: Newz failed");

        if (eax_init(&RETVAL->state, id, k, (unsigned long)k_len, n, (unsigned long)n_len, h, (unsigned long)h_len) != CRYPT_OK) {
          croak("FATAL: eax setup failed");
        }
    }
    OUTPUT:
        RETVAL

void
DESTROY(Crypt::AuthEnc::EAX self)
    CODE:
        Safefree(self);

Crypt::AuthEnc::EAX
clone(Crypt::AuthEnc::EAX self)
    CODE:
        Newz(0, RETVAL, 1, struct eax_struct);
        if (!RETVAL) croak("FATAL: Newz failed");
        Copy(&self->state, &RETVAL->state, 1, struct eax_struct);
    OUTPUT:
        RETVAL

SV *
encrypt_add(Crypt::AuthEnc::EAX self, SV * data)
    CODE:
    {
        int rv;
        STRLEN in_data_len;
        unsigned char *in_data, *out_data;

        in_data = (unsigned char *)SvPVbyte(data, in_data_len);
        if (in_data_len==0) {
          RETVAL = newSVpvn("", 0);
        }
        else {
          RETVAL = NEWSV(0, in_data_len);
          SvPOK_only(RETVAL);
          SvCUR_set(RETVAL, in_data_len);
          out_data = (unsigned char *)SvPV_nolen(RETVAL);
          rv = eax_encrypt(&self->state, in_data, out_data, (unsigned long)in_data_len);
          if (rv != CRYPT_OK) croak("FATAL: eax_encrypt failed: %s", error_to_string(rv));
        }
    }
    OUTPUT:
        RETVAL

SV *
decrypt_add(Crypt::AuthEnc::EAX self, SV * data)
    CODE:
    {
        int rv;
        STRLEN in_data_len;
        unsigned char *in_data, *out_data;

        in_data = (unsigned char *)SvPVbyte(data, in_data_len);
        if (in_data_len==0) {
          RETVAL = newSVpvn("", 0);
        }
        else {
          RETVAL = NEWSV(0, in_data_len);
          SvPOK_only(RETVAL);
          SvCUR_set(RETVAL, in_data_len);
          out_data = (unsigned char *)SvPV_nolen(RETVAL);
          rv = eax_decrypt(&self->state, in_data, out_data, (unsigned long)in_data_len);
          if (rv != CRYPT_OK) croak("FATAL: eax_decrypt failed: %s", error_to_string(rv));
        }
    }
    OUTPUT:
        RETVAL

void
encrypt_done(Crypt::AuthEnc::EAX self)
    PPCODE:
    {
        int rv;
        unsigned char tag[MAXBLOCKSIZE];
        unsigned long tag_len = sizeof(tag);

        rv = eax_done(&self->state, tag, &tag_len);
        if (rv != CRYPT_OK) croak("FATAL: eax_done failed: %s", error_to_string(rv));
        XPUSHs(sv_2mortal(newSVpvn((char*)tag, tag_len)));
    }

void
decrypt_done(Crypt::AuthEnc::EAX self, ...)
    PPCODE:
    {
        int rv;
        unsigned char tag[MAXBLOCKSIZE];
        unsigned long tag_len = sizeof(tag);
        STRLEN expected_tag_len;
        unsigned char *expected_tag;

        rv = eax_done(&self->state, tag, &tag_len);
        if (rv != CRYPT_OK) croak("FATAL: eax_done failed: %s", error_to_string(rv));
        if (items == 1) {
          XPUSHs(sv_2mortal(newSVpvn((char*)tag, tag_len)));
        }
        else {
          if(!SvPOK(ST(1))) croak("FATAL: expected_tag must be string/buffer scalar");
          expected_tag = (unsigned char *) SvPVbyte(ST(1), expected_tag_len);
          if (expected_tag_len!=tag_len) {
            XPUSHs(sv_2mortal(newSViv(0))); /* false */
          }
          else if (memNE(expected_tag, tag, tag_len)) {
            XPUSHs(sv_2mortal(newSViv(0))); /* false */
          }
          else {
            XPUSHs(sv_2mortal(newSViv(1))); /* true */
          }
        }
    }

int
header_add(Crypt::AuthEnc::EAX self, SV * header)
    CODE:
    {
        STRLEN h_len;
        unsigned char *h;
        h = (unsigned char *)SvPVbyte(header, h_len);
        RETVAL = eax_addheader(&self->state, h, (unsigned long)h_len);
    }
    OUTPUT:
        RETVAL
