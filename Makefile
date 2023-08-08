include $(TOPDIR)/rules.mk

PKG_NAME:=esp_app
PKG_RELEASE:=1
PKG_VERSION:=1.2.12

include $(INCLUDE_DIR)/package.mk

define Package/esp_app
	CATEGORY:=Extra packages
	TITLE:=esp_app
	DEPENDS:=+libserialport +libubus +libubox +libblobmsg-json +libtuyasdk
endef

define Package/esp_app/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/esp_app $(1)/usr/bin
	$(INSTALL_BIN) ./files/esp_app.init $(1)/etc/init.d/esp_app
	$(INSTALL_CONF) ./files/esp_app.config $(1)/etc/config/esp_app

endef

$(eval $(call BuildPackage,esp_app))
