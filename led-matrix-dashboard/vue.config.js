const CompressionPlugin = require('compression-webpack-plugin');

let assetsDir = "assets";
module.exports = {
  configureWebpack: {
    output: {
      filename: 'js/[id].js',
      chunkFilename: 'js/[id].js',
    }
  },
  chainWebpack: config => {
    if (config.plugins.has("extract-css")) {
      const extractCSSPlugin = config.plugin("extract-css");
      extractCSSPlugin &&
      extractCSSPlugin.tap(() => [
        {
          filename: assetsDir + "/[name].css",
          chunkFilename: assetsDir + "/[name].css"
        }
      ]);
    }

    config.plugins
      //.delete("html")
      .delete("prefetch")
      .delete("preload");
    config.plugin('CompressionPlugin').use(CompressionPlugin);
  }
};
