function get_path() {
    return window.location.pathname;
}

let last_path = '';
let last_path_page = '';
function set_path(path) {
    last_path = get_path();
    window.history.pushState({}, '', path);
}

function append_to_header(element) {
    document.getElementsByTagName('head').item(0).appendChild(element);
}

function get_cookie(name) {
    const name_eq = `${name}=`;
    const ca = document.cookie.split(';');
    for (let i = 0; i < ca.length; i++) {
        let c = ca[i];
        while (c.charAt(0) === ' ') {
            c = c.substring(1, c.length);
        }
        if (c.indexOf(name_eq) === 0) {
            return c.substring(name_eq.length, c.length);
        }
    }
    return null;
}

function clear_cookies() {
    const cookies = document.cookie.split(';');
    for (let i = 0; i < cookies.length; i++) {
        const cookie = cookies[i];
        const eq = cookie.indexOf('=');
        const name = eq > -1 ? cookie.substr(0, eq).trim() : cookie.trim();
        document.cookie = `${name}=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/; domain=${window.location.hostname}`;
    }
}

function include(file) {
    const script = document.createElement('script');
    script.src = file;
    script.type = 'text/javascript';
    script.defer = true;
    append_to_header(script);
}

async function get_site_settings() {
    try {
        const response = await fetch('/api/get_settings');
        if (!response.ok) {
            throw new Error('network response was not ok');
        }
        const settings = await response.json();
        return settings;
    } catch (error) {
        console.error('there was a problem with the fetch operation:', error);
    }
}

class WindowProperties {
    constructor({
                    close_button = true,
                    moveable = false,
                    close_on_click_outside = false,
                    close_on_escape = true,
                    remove_existing = true,
                    function_on_close = function() {
                        if (last_path_page) {
                            set_path(last_path_page);
                            load_page(last_path_page);
                        }
                    }
                } = {}) {
        this.close_button = close_button;
        this.moveable = moveable;
        this.close_on_click_outside = close_on_click_outside;
        this.close_on_escape = close_on_escape;
        this.remove_existing = remove_existing;
        this.function_on_close = function_on_close;
    }
}

function hide_all_windows() {
    const windows = document.getElementsByClassName('floating_window');
    for (let i = 0; i < windows.length; i++) {
        windows[i].style.display = 'none';
        while (windows[i].firstChild) {
            windows[i].removeChild(windows[i].firstChild);
        }
    }
}

function create_window(id, prop = new WindowProperties()) {
    const hide_all_windows = () => {
        const windows = document.getElementsByClassName('floating_window');
        for (let i = 0; i < windows.length; i++) {
            windows[i].style.display = 'none';
            while (windows[i].firstChild) {
                windows[i].removeChild(windows[i].firstChild);
            }
        }
    }
    if (prop.remove_existing) {
        hide_all_windows();
    }

    const existing = document.getElementById(id);
    if (existing) {
        existing.style.display = 'none';
        while (existing.firstChild) {
            existing.removeChild(existing.firstChild);
        }
    }
    const content = document.createElement('div');
    content.className = 'floating_window';
    content.id = id;
    if (prop.close_on_click_outside) {
        content.onclick = (event) => {
            if (event.target === content) {
                if (prop.function_on_close) {
                    prop.function_on_close();
                }
                hide_all_windows();
            }
        }
    }
    if (prop.close_on_escape) {
        document.onkeydown = (event) => {
            if (event.key === 'Escape') {
                if (prop.function_on_close) {
                    prop.function_on_close();
                }
                hide_all_windows();
            }
        }
    }

    content.oncontextmenu = (event) => {
        event.preventDefault();
    }

    let xpos = 0;
    let ypos = 0;
    let offsetX = 0;
    let offsetY = 0;

    if (prop.moveable) {
        content.onmousedown = (event) => {
            xpos = event.clientX;
            ypos = event.clientY;
            offsetX = xpos - content.offsetLeft;
            offsetY = ypos - content.offsetTop;

            document.onmousemove = (event) => {
                content.style.left = (event.clientX - offsetX) + 'px';
                content.style.top = (event.clientY - offsetY) + 'px';
            }
        }

        document.onmouseup = () => {
            document.onmousemove = null;
        }
    }

    if (prop.close_button) {
        const close = document.createElement('a');
        close.innerHTML = '✕';
        close.id = 'window-close';
        close.style.position = 'fixed';
        close.style.padding = '10px';
        close.style.top = '0';
        close.style.right = '0';
        close.style.textDecoration = 'none';
        close.onclick = () => {
            if (prop.function_on_close) {
                prop.function_on_close();
            }
            hide_all_windows();
        }

        content.appendChild(close);
    }

    document.body.appendChild(content);

    return content;
}

async function is_username(username) {
    const api = "/api/user_exists";
    const data = { username: username };

    try {
        const response = await fetch(api, {
            headers: {
                'Content-Type': 'application/json'
            },
            method: 'POST',
            body: JSON.stringify(data),
        });
        const result = await response.json();
        return result.status !== undefined ? result.status : true;
    } catch (error) {
        console.error('Error:', error);
        return true;
    }
}

async function login(data, error = "") {
    const try_login = (data, username, password) => {
        let send = {};
        if (username.includes('@')) {
            send.email = username;
        } else {
            send.username = username;
        }
        send.password = password;

        fetch("/api/try_login", {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(send),
        })
            .then(response => response.json())
            .then(json => {
                if (json.error_str) {
                    login(data, json.error_str);
                    return;
                } else if (json.key) {
                    window.location.href = '/';
                    return;
                }

                throw new Error('Invalid response from server: ' + JSON.stringify(json));
            })
            .catch((error) => {
                console.error('Error:', error);
            });
    }

    const password_prompt = () => {
        const username = document.getElementById('login_username').value;

        const window = create_window('login-window');
        const title = document.createElement('h1');
        title.id = 'login_title';
        title.classList = 'login-title';
        title.innerText = 'Enter your password!';

        const paragraph = document.createElement('p');
        paragraph.id = 'login_paragraph';
        paragraph.classList = 'login-paragraph';
        paragraph.innerText = 'Please enter your password to continue.';

        const button = document.createElement('button');
        button.id = 'login_button';
        button.innerText = 'Login';
        button.enabled = false;
        button.onclick = (event) => {
            try_login(data, username, password.value);
        }

        const password = document.createElement('input');
        password.id = 'login_password';
        password.classList = 'login-password';
        password.type = 'password';
        password.placeholder = 'Password';
        password.oninput = () => {
            button.enabled = password.value !== "";
        }

        window.appendChild(title);
        window.appendChild(paragraph);
        window.appendChild(document.createElement('br'));
        window.appendChild(password);
        window.appendChild(document.createElement('br'));
        window.appendChild(document.createElement('br'));
        window.appendChild(button);
    }

    const username_prompt = async () => {
        const window = create_window('login-window');
        const title = document.createElement('h1');
        title.id = 'login_title';
        title.classList = 'login-title';
        title.innerText = 'Welcome back!';

        const paragraph = document.createElement('p');
        paragraph.id = 'login_paragraph';
        paragraph.classList = 'login-paragraph';
        paragraph.innerHTML = 'Welcome back to ' + data.general.site + '. Please login to continue.<br>';
        const link = document.createElement('a');
        link.onclick = () => {
            register(data);
        }
        link.id = 'login_register_link';
        link.classList = 'login-register-link';
        link.innerText = "Don't have an account? Register here!";
        paragraph.appendChild(link);

        const username = document.createElement('input');
        username.id = 'login_username';
        username.type = 'text';
        username.placeholder = 'Username or email address';
        username.oninput = () => {
            const errors = document.getElementsByClassName('error');
            for (let i = 0; i < errors.length; i++) {
                const error = errors[i];
                window.removeChild(error);
            }
        }

        const button = document.createElement('button');
        button.id = 'login_button';
        button.innerText = 'Continue';
        button.enabled = false;
        button.onclick = () => {
            password_prompt();
        }

        username.oninput = async () => {
            if (username.value === "") {
                button.enabled = false;
            } else {
                button.enabled = true;
            }
        }

        window.appendChild(title);
        window.appendChild(paragraph);
        window.appendChild(document.createElement('br'));
        window.appendChild(username);
        window.appendChild(document.createElement('br'));
        window.appendChild(document.createElement('br'));
        window.appendChild(button);

        if (error !== "") {
            const error_message = document.createElement('p');
            error_message.id = 'login-error';
            error_message.classList = 'error login-error';
            error_message.innerText = error;
            window.appendChild(error_message);
        }
    }

    username_prompt();
}

function logout(data) {
    const win = create_window('logout-window');

    const title = document.createElement('h1');
    const paragraph = document.createElement('p');

    title.innerText = 'Log out';
    title.id = 'logout-title';
    title.classList = 'logout_title';

    paragraph.innerText = 'Are you sure you want to log out?';
    paragraph.id = 'logout-paragraph';
    paragraph.classList = 'logout_paragraph';

    const button = document.createElement('button');
    button.innerHTML = 'Yes, log me out!';
    button.onclick = () => {
        clear_cookies();
        window.location = "/";
    }

    win.appendChild(title);
    win.appendChild(paragraph)
    win.appendChild(button);
}

function register(data, error = "") {
    let stat = {};

    const try_register = async () => {
        const api = "/api/register";
        const send = {
            username: stat.username,
            email: stat.email,
            password: stat.password
        }


        fetch(api, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(send),
        })
            .then(response => {
                if (response.status === 204) { // success without email verification needed
                    hide_all_windows();
                    return;
                }

                return response.json();
            })
            .then(json => {
                if (json && json.error_str) {
                    register(data, json.error_str);
                    return;
                }

                if (json) {
                    throw new Error('Invalid response from server: ' + JSON.stringify(json));
                }
            })
            .catch((error) => {
                console.error('Error:', error);
            });
    }

    const confirm_prompt = async () => {
        stat.password = document.getElementById('register_password').value;

        const window = create_window('register-window');
        const title = document.createElement('h1');
        title.id = 'register_title';
        title.classList = 'register-title';
        title.innerText = 'Confirm your password';

        const paragraph = document.createElement('p');
        paragraph.id = 'register_paragraph';
        paragraph.classList = 'register-paragraph';
        paragraph.innerHTML = 'Please re-enter the password you just entered. This is to ensure that you did not make any mistakes, and that you can remember your password.';

        const confirm = document.createElement('input');
        confirm.id = 'register_confirm';
        confirm.classList = 'register-confirm';
        confirm.type = 'password';
        confirm.placeholder = 'Confirm your password';

        const button = document.createElement('button');
        button.id = 'register_button';
        button.innerText = 'Register';

        button.onclick = async () => {
            if (confirm.value !== stat.password) {
                register(data, 'Passwords do not match.');
                return;
            }

            try_register();
        }

        window.appendChild(title);
        window.appendChild(paragraph);
        window.appendChild(document.createElement('br'));
        window.appendChild(confirm);
        window.appendChild(document.createElement('br'));
        window.appendChild(document.createElement('br'));
        window.appendChild(button);
    }

    const password_prompt = async () => {
        stat.email = document.getElementById('register_email').value;

        const window = create_window('register-window');
        const title = document.createElement('h1');
        title.id = 'register_title';
        title.classList = 'register-title';
        title.innerText = 'Password';

        const paragraph = document.createElement('p');
        paragraph.id = 'register_paragraph';
        paragraph.classList = 'register-paragraph';
        paragraph.innerHTML = 'Please enter a password for this account. A good password is at least 8 characters long and contains a mix of letters, numbers, and symbols. Please do not use a password you use elsewhere or have used in the past.';

        const password = document.createElement('input');
        password.id = 'register_password';
        password.classList = 'register-password';
        password.type = 'password';
        password.placeholder = 'Password';

        const button = document.createElement('button');
        button.id = 'register_button';
        button.innerText = 'Continue';

        button.onclick = async () => {
            if (password.value === "") {
                register(data, 'Please enter a password.');
                return;
            } else if (password.value.length < 8) {
                register(data, 'Password must be at least 8 characters long.');
                return;
            }

            confirm_prompt();
        }

        window.appendChild(title);
        window.appendChild(paragraph);
        window.appendChild(document.createElement('br'));
        window.appendChild(password);
        window.appendChild(document.createElement('br'));
        window.appendChild(document.createElement('br'));
        window.appendChild(button);
    }

    const email_prompt = async () => {
        stat.username = document.getElementById('register_username').value;

        const window = create_window('register-window');
        const title = document.createElement('h1');
        title.id = 'register_title';
        title.classList = 'register-title';
        title.innerText = 'Email address';

        const paragraph = document.createElement('p');
        paragraph.id = 'register_paragraph';
        paragraph.classList = 'register-paragraph';
        paragraph.innerHTML = 'Please enter your email address. It will be used in the event that we need to urgently contact you. No spam, we promise!';

        const email = document.createElement('input');
        email.id = 'register_email';
        email.classList = 'register-email';
        email.type = 'email';
        email.placeholder = 'Email address';

        const button = document.createElement('button');
        button.id = 'register_button';
        button.innerText = 'Continue';
        button.onclick = async () => {
            if (email.value === "") {
                register(data, 'Please enter your email address.');
                return;
            } else if (email.value.includes('@') == false) {
                register(data, 'That does not look like an email address.');
                return;
            } else if (await is_username(email.value)) {
                register(data, 'Email address already exists.');
                return;
            }

            password_prompt();
        }

        window.appendChild(title);
        window.appendChild(paragraph);
        window.appendChild(document.createElement('br'));
        window.appendChild(email);
        window.appendChild(document.createElement('br'));
        window.appendChild(document.createElement('br'));
        window.appendChild(button);
    }

    const username_prompt = async () => {
        const window = create_window('register-window');
        const title = document.createElement('h1');
        title.id = 'register_title';
        title.classList = 'register-title';
        title.innerText = 'Welcome!';

        const paragraph = document.createElement('p');
        paragraph.id = 'register_paragraph';
        paragraph.classList = 'register-paragraph';
        paragraph.innerHTML = 'Welcome to ' + data.general.site + '.<br>';
        const link = document.createElement('a');
        link.onclick = () => {
            register(data);
        }
        link.id = 'register_login_link';
        link.classList = 'register-login-link';
        link.innerText = "Already have an account? Log in here!";
        paragraph.appendChild(link);

        const username = document.createElement('input');
        username.id = 'register_username';
        username.type = 'text';
        username.placeholder = 'Username';
        username.oninput = () => {
            const errors = document.getElementsByClassName('error');
            for (let i = 0; i < errors.length; i++) {
                const error = errors[i];
                window.removeChild(error);
            }
        }

        const button = document.createElement('button');
        button.id = 'register_button';
        button.innerText = 'Continue';
        button.enabled = false;
        button.onclick = async () => {
            if (await is_username(username.value)) {
                register(data, 'Username already exists.');
                return;
            }
            if (username.value.includes('@')) {
                register(data, 'That looks like an email address. Please use a username.');
                return;
            }
            email_prompt()
        }
        username.oninput = async () => {
            if (username.value === "") {
                button.enabled = false;
            } else {
                button.enabled = true;
            }
        }

        window.appendChild(title);
        window.appendChild(paragraph);
        window.appendChild(document.createElement('br'));
        window.appendChild(username);
        window.appendChild(document.createElement('br'));
        window.appendChild(document.createElement('br'));
        window.appendChild(button);

        if (error !== "") {
            const error_message = document.createElement('p');
            error_message.id = 'register-error';
            error_message.classList = 'error register-error';
            error_message.innerText = error;
            window.appendChild(error_message);
        }
    }

    username_prompt();
}

async function delete_page(data, page) {
    const api = "/api/delete_page";
    const send = { page: page };

    try {
        const response = await fetch(api, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(send),
        });

        if (response.status === 204) {
            return;
        }

        const json = await response.json();

        if (json.error_str) {
            console.error('Error:', json.error_str);
            return;
        }
    } catch (error) {
        console.error('Error:', error);
    }
}

function delete_page_window(data, pages) {
    const win = create_window('delete-window');

    const title = document.createElement('h1');
    title.id = 'delete-title';
    title.classList = 'delete-title';
    title.innerText = 'Delete page';

    const paragraph = document.createElement('p');
    paragraph.id = 'delete-paragraph';
    paragraph.classList = 'delete-paragraph';
    paragraph.innerText = 'Are you sure you want to delete the following page(s)?';

    const list = document.createElement('ul');
    list.id = 'delete-list';
    list.classList = 'delete-list';
    for (const page of pages) {
        const item = document.createElement('li');
        item.innerText = page;
        list.appendChild(item);
    }

    const button = document.createElement('button');
    button.id = 'delete-button';
    button.innerText = 'Delete';
    button.onclick = async () => {
        for (const page of pages) {
            await delete_page(data, page);
        }

        page_manage(data);
    }

    const cancel = document.createElement('button');
    cancel.id = 'delete-cancel';
    cancel.innerText = 'Cancel';
    cancel.onclick = () => {
        if (pages.length === 1) {
            manage_page(data, pages[0]);
        } else {
            page_manage(data);
        }
    }

    win.appendChild(title);
    win.appendChild(paragraph);
    win.appendChild(list);
    win.appendChild(button);
    win.appendChild(cancel);
}

function spawn_editor(input_title, input_md, on_save, on_cancel) {
    let last_saved = 0;
    const win = create_window('editor-window');

    const title = document.createElement('h1');
    title.id = 'editor-title';
    title.classList = 'editor-title';
    title.innerText = input_title;

    const textarea = document.createElement('textarea');
    textarea.id = 'editor-textarea';
    textarea.classList = 'editor-textarea';
    textarea.style.minWidth = '600px';
    textarea.style.minHeight = '300px';
    textarea.value = input_md;

    const cancel = document.createElement('button');
    cancel.id = 'editor-cancel';
    cancel.innerText = last_saved == 0 ? 'Cancel' : 'Finish';
    cancel.onclick = () => {
        if (last_saved == 0) {
            if (textarea.value !== input_md) {
                const text = textarea.value;
                const new_win = create_window('confirm-window', {remove_existing: false});
                const title = document.createElement('h1');
                title.id = 'confirm-title';
                title.classList = 'confirm-title';
                title.innerText = 'Unsaved changes';
                const paragraph = document.createElement('p');
                paragraph.id = 'confirm-paragraph';
                paragraph.classList = 'confirm-paragraph';
                paragraph.innerText = 'You have unsaved changes. Are you sure you want to cancel?';
                const confirm = document.createElement('button');
                confirm.id = 'confirm-confirm';
                confirm.innerText = 'Yes';
                confirm.onclick = () => {
                    new_win.style.display = 'none';
                    on_cancel();
                }
                const no = document.createElement('button');
                no.id = 'confirm-no';
                no.innerText = 'No';
                no.onclick = () => {
                    editor(title, text, on_save, on_cancel);
                }

                new_win.appendChild(title);
                new_win.appendChild(paragraph);
                new_win.appendChild(no);
                new_win.appendChild(confirm);
            }
        }

        on_cancel();
    }

    const save = document.createElement('button');
    save.id = 'editor-save';
    save.innerText = 'Save';
    save.onclick = () => {
        last_saved = Date.now();
        on_save(textarea.value);
    }

    win.appendChild(textarea);
    if (last_saved != 0) {
        const last_saved_text = document.createElement('p');
        last_saved_text.id = 'editor-last-saved';
        last_saved_text.classList = 'editor-last-saved';
        last_saved_text.innerText = 'Last saved ' + last_saved;
        win.appendChild(last_saved_text);
    }
    win.appendChild(document.createElement('br'));
    win.appendChild(document.createElement('br'));
    win.appendChild(save);
    win.appendChild(cancel);
}

function edit_page_window(data, page) {
    async function save_page(page, content) {
        const api = "/api/update_page";
        const send = { page: page, markdown_content: content };

        try {
            const response = await fetch(api, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(send),
            });

            if (response.status === 204) {
                return;
            }

            const json = await response.json();

            if (json.error_str) {
                console.error('Error:', json.error_str);
                return;
            }
        } catch (error) {
            console.error('Error:', error);
        }
    }

    async function get_page(page) {
        const api = "/api/get_page";
        const send = { page: page };

        try {
            const response = await fetch(api, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(send),
            });

            const json = await response.json();

            if (json.error_str) {
                console.error('Error:', json.error_str);
                return;
            }

            return json;
        } catch (error) {
            console.error('Error:', error);
        }
    }

    get_page(page).then((json) => {
        spawn_editor('Edit page ' + page, json.input_content, (content) => {
            save_page(page, content);
        }, () => {
            manage_page(data, page);
        });
    });
}

function create_page_editor(data, page) {
    async function save_page(page, content) {
        const api = "/api/create_page";
        const send = { page: page, markdown_content: content };

        try {
            const response = await fetch(api, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(send),
            });

            if (response.status === 204) {
                manage_page(data, page);
                return;
            }

            const json = await response.json();

            if (json.error_str) {
                console.error('Error:', json.error_str);
                return;
            }
        } catch (error) {
            console.error('Error:', error);
        }
    }

    const input_md = '# ' + page + '\n\nThis is a new page. You can fill me with Markdown.';
    spawn_editor('Create new page', input_md, (content) => {
        save_page(page, content);
    }, () => {
        page_manage(data);
    });
}

function create_page_window(data) {
    const win = create_window('page-window');

    const title = document.createElement('h1');
    title.id = 'page-title'
    title.classList = 'page-title';
    title.innerText = 'Create new page';

    const paragraph = document.createElement('p');
    paragraph.id = 'page-paragraph';
    paragraph.classList = 'page-paragraph';
    paragraph.innerText = 'Please choose an endpoint for your new page. This will be the URL that users will visit to access your page. e.g. /about';

    const endpoint = document.createElement('input');
    endpoint.id = 'page-endpoint';
    endpoint.classList = 'page-endpoint';
    endpoint.placeholder = 'Endpoint (e.g. /about)';

    const continue_button = document.createElement('button');
    continue_button.id = 'page-continue';
    continue_button.innerText = 'Continue';
    continue_button.onclick = async () => {
        create_page_editor(data, endpoint.value);
    }

    const back = document.createElement('button');
    back.id = 'page-back';
    back.innerText = 'Back';
    back.onclick = () => {
        page_manage(data);
    }

    win.appendChild(title);
    win.appendChild(paragraph);
    win.appendChild(endpoint);
    win.appendChild(document.createElement('br'));
    win.appendChild(document.createElement('br'));
    win.appendChild(back);
    win.appendChild(continue_button);
}

function manage_page(data, page) {
    set_path('/admin/page/' + page);

    const win = create_window('page-window');

    const preview = document.createElement('iframe');
    preview.id = 'page-preview';
    preview.classList = 'page-preview';
    preview.src = page;
    preview.style.transform = 'scale(0.8)';
    preview.style.transformOrigin = '0 0';
    preview.style.width = '500px';
    preview.style.height = '500px';

    preview.onload = () => {
        preview.contentWindow.document.getElementById('topBar').style.display = 'none';
        preview.contentWindow.document.getElementById('footer').style.display = 'none';
    }

    const title = document.createElement('h1');
    title.id = 'page-title';
    title.innerHTML = 'Manage page ' + page;

    const paragraph = document.createElement('p');
    paragraph.id = 'page-paragraph';
    paragraph.classList = 'page-paragraph';
    paragraph.innerHTML = 'Manage your page here.';

    const edit_button = document.createElement('button');
    edit_button.id = 'page-edit';
    edit_button.innerText = 'Edit page';
    edit_button.onclick = async () => {
        edit_page_window(data, page);
    }

    const delete_button = document.createElement('button');
    delete_button.id = 'page-delete';
    delete_button.innerText = 'Delete page';
    delete_button.onclick = async () => {
        delete_page_window(data, page);
    }

    win.appendChild(preview);
    win.appendChild(title);
    win.appendChild(paragraph);
    win.appendChild(edit_button);
    win.appendChild(delete_button);
}

async function page_manage(data) {
    const win = create_window('page-window');

    set_path('/admin/page');

    async function get_pages() {
        const api = "/api/get_hierarchy";
        let list = [];

        try {
            const response = await fetch(api, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
            });
            const json = await response.json();
            for (const page in json) {
                if (json[page].type === 'page') {
                    list.push(page);
                }
            }
        } catch (error) {
            console.error('Error:', error);
        }

        return list;
    }

    const title = document.createElement('h1');
    const paragraph = document.createElement('p');

    title.innerHTML = 'Manage pages';
    paragraph.innerHTML = 'Manage your web pages here.';

    const table = document.createElement('table');
    table.classList = 'page-table';

    const create_page = document.createElement('button');
    create_page.innerText = 'Create new page';
    create_page.onclick = () => {
        create_page_window(data);
    }

    function print_list(data, list) {
        let to_delete = [];
        while (table.firstChild) {
            table.removeChild(table.firstChild);
        }

        if (list.length === 0) {
            const row = document.createElement('tr');
            const cell = document.createElement('td');
            cell.innerText = 'No pages found.';
            row.appendChild(cell);
            table.appendChild(row);
            return;
        }

        const check_all = document.createElement('input');
        check_all.type = 'checkbox';
        check_all.id = 'page-all';
        check_all.name = 'page-all';
        check_all.checked = false;
        check_all.onchange = () => {
            const checks = document.getElementsByTagName('input');
            for (let i = 0; i < checks.length; i++) {
                if (checks[i].type === 'checkbox' && checks[i].id !== 'page-all') {
                    checks[i].checked = check_all.checked;
                }
            }

            to_delete = [];
            if (check_all.checked) {
                for (const page of list) {
                    to_delete.push(page);
                }
            }
        }

        const check_all_label = document.createElement('label');
        check_all_label.htmlFor = 'page-all';
        check_all_label.innerText = 'Select all';

        const check_all_cell = document.createElement('td');
        check_all_cell.appendChild(check_all);
        check_all_cell.appendChild(check_all_label);

        const check_all_row = document.createElement('tr');
        check_all_row.appendChild(check_all_cell);

        const delete_checked = document.createElement('button');
        delete_checked.innerText = 'Delete selected';
        delete_checked.onclick = async () => {
            if (to_delete.length === 0) {
                return;
            }

            delete_page_window(data, to_delete);
        }

        check_all_row.appendChild(delete_checked);

        table.appendChild(check_all_row);

        for (const page of list) {
            const row = document.createElement('tr');
            const cell = document.createElement('td');

            const check = document.createElement('input');
            check.type = 'checkbox';
            check.id = 'page-' + page;
            check.name = 'page-' + page;
            check.checked = false;
            check.onchange = () => {
                // uncheck select all if all are not checked
                const checks = document.getElementsByTagName('input');
                let all_checked = true;
                for (let i = 0; i < checks.length; i++) {
                    if (checks[i].type === 'checkbox' && checks[i].id !== 'page-all' && checks[i].checked === false) {
                        all_checked = false;
                        break;
                    }
                }
                check_all.checked = all_checked;

                // add or remove from to_delete
                if (check.checked) {
                    to_delete.push(page);
                } else {
                    const index = to_delete.indexOf(page);
                    if (index > -1) {
                        to_delete.splice(index, 1);
                    }
                }
            }
            cell.appendChild(check);

            const link = document.createElement('a');
            link.onclick = () => {
                manage_page(data, page);
            }

            link.innerText = page;

            cell.appendChild(link);

            const delete_button = document.createElement('button');
            delete_button.innerText = 'Delete';
            delete_button.onclick = async () => {
                delete_page_window(data, [page]);
            }

            const edit_button = document.createElement('button');
            edit_button.innerText = 'Edit';
            edit_button.onclick = async () => {
                edit_page_window(data, page);
            }

            cell.appendChild(edit_button);
            cell.appendChild(delete_button);

            row.appendChild(cell);
            table.appendChild(row);
        }
    }

    print_list(data, await get_pages());

    const search = document.createElement('input');
    search.placeholder = 'Search';
    search.oninput = async () => {
        const search_results = await get_pages();

        // remove entries that don't match the search
        for (let i = 0; i < search_results.length; i++) {
            if (!search_results[i].includes(search.value)) {
                search_results.splice(i, 1);
                i--;
            }
        }

        print_list(data, search_results);
    }

    win.appendChild(title);
    win.appendChild(paragraph);
    win.appendChild(create_page);
    win.appendChild(document.createElement('br'));
    win.appendChild(document.createElement('br'));
    win.appendChild(search);
    win.appendChild(document.createElement('br'));
    win.appendChild(document.createElement('br'));
    win.appendChild(table);
}

function upload_files(files, base_endpoint, is_public = true) {
    const loading = create_window('loading-window', { close_button: false, moveable: false, remove_existing: true, close_on_escape: false, close_on_click_outside: false });

    const title = document.createElement('h1');
    title.innerHTML = 'Uploading...';

    const paragraph = document.createElement('p');
    paragraph.innerHTML = 'Please wait while we upload your files. This may take a few moments...';

    loading.appendChild(title);
    loading.appendChild(paragraph);

    const url = '/api/upload_file';

    for (const file of files) {
        let json = {};
        const form = new FormData();
        let endpoint = base_endpoint;

        if (files.length > 1) { // more than one file
            endpoint += "/" + file.name;
        }

        // prevent multiple slashes
        endpoint = endpoint.replace(/\/+/g, '/');

        json.require_admin = !is_public;
        json.endpoint = endpoint;

        form.append('json', new Blob([JSON.stringify(json)], {type: 'application/json'}));
        form.append('file', file);

        fetch(url, {
            method: 'POST',
            body: form,
        })
            .then(response => {
                if (response.status === 204) {
                    return;
                }

                return response.json();
            })
            .then(json => {
                if (json) {
                    throw new Error('Invalid response from server: ' + JSON.stringify(json));
                }
            })
            .catch((error) => {
                console.error('Error:', error);
            });
    }
}

function upload_file_window(data) {
    const win = create_window('upload-window');

    const title = document.createElement('h1');
    const paragraph = document.createElement('p');

    title.innerHTML = 'Upload file';
    title.id = 'upload-title';

    paragraph.innerText = 'Upload a file here. If you select multiple files, the endpoint will be used as a directory, with the filename as the file name.';
    paragraph.id = 'upload-paragraph';

    const endpoint = document.createElement('input');
    endpoint.id = 'upload-endpoint';
    endpoint.placeholder = "Endpoint (e.g. '/files/myfile.txt')";

    const public_check = document.createElement('input');
    public_check.id = 'upload-public';
    public_check.type = 'checkbox';
    public_check.checked = true;

    const public_label = document.createElement('label');
    public_label.innerHTML = 'Public';
    public_label.id = 'upload-public-label';

    const upload = document.createElement('input');
    upload.id = 'upload-file';
    upload.type = 'file';
    upload.multiple = true;

    const button = document.createElement('button');
    button.id = 'upload-button';
    button.innerText = 'Upload';
    button.onclick = () => {
        if (upload.files.length === 0) {
            return;
        }
        upload_files(upload.files, endpoint.value, public_check.checked);
        file_manage(data);
    }

    win.appendChild(title);
    win.appendChild(paragraph);
    win.appendChild(endpoint);
    win.appendChild(upload);
    win.appendChild(public_label);
    win.appendChild(public_check);
    win.appendChild(button);
}

async function delete_file(data, file) {
    const api = "/api/delete_file";
    const send = { endpoint: file };

    try {
        const response = await fetch(api, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(send),
        });

        if (response.status === 204) {
            return;
        }

        const json = await response.json();

        if (json.error_str) {
            console.error('Error:', json.error_str);
            return;
        }
    } catch (error) {
        console.error('Error:', error);
    }
}

function delete_file_window(data, files) {
    const win = create_window('delete-window');

    const title = document.createElement('h1');
    title.id = 'delete-title';
    title.classList = 'delete-title';
    title.innerText = 'Delete file';

    const paragraph = document.createElement('p');
    paragraph.id = 'delete-paragraph';
    paragraph.classList = 'delete-paragraph';
    paragraph.innerText = 'Are you sure you want to delete the following file(s)?';

    const list = document.createElement('ul');
    list.id = 'delete-list';
    list.classList = 'delete-list';
    for (const file of files) {
        const item = document.createElement('li');
        item.innerText = file;
        list.appendChild(item);
    }

    const button = document.createElement('button');
    button.id = 'delete-button';
    button.innerText = 'Delete';
    button.onclick = async () => {
        for (const file of files) {
            await delete_file(data, file);
        }

        file_manage(data);
    }

    const cancel = document.createElement('button');
    cancel.id = 'delete-cancel';
    cancel.innerText = 'Cancel';
    cancel.onclick = () => {
        if (files.length === 1) {
            manage_file(data, pages[0]);
        } else {
            file_manage(data);
        }
    }

    win.appendChild(title);
    win.appendChild(paragraph);
    win.appendChild(list);
    win.appendChild(button);
    win.appendChild(cancel);
}

function manage_file(data, file) {
    set_path('/admin/file/' + file);

    const win = create_window('file-window');

    const title = document.createElement('h1');
    title.id = 'file-title';
    title.innerHTML = 'Manage file ' + file;

    const paragraph = document.createElement('p');
    paragraph.id = 'file-paragraph';
    paragraph.classList = 'file-paragraph';
    paragraph.innerHTML = 'Manage your file here.';

    const delete_button = document.createElement('button');
    delete_button.id = 'file-delete';
    delete_button.innerText = 'Delete file';
    delete_button.onclick = async () => {
        delete_page_window(data, file);
    }

    win.appendChild(title);
    win.appendChild(paragraph);
    win.appendChild(delete_button);
}

// pretty much the same as the page management at first glance
// in fact, i'll copy paste pretty much half the function
async function file_manage(data) {
    async function get_files() {
        const api = "/api/get_hierarchy";
        let list = [];

        try {
            const response = await fetch(api, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
            });
            const json = await response.json();
            for (const file in json) {
                if (json[file].type === 'file') {
                    list.push(file);
                }
            }
        } catch (error) {
            console.error('Error:', error);
        }

        return list;
    }

    set_path('/admin/file');

    const win = create_window('file-window');

    const title = document.createElement('h1');
    const paragraph = document.createElement('p');

    title.innerHTML = 'Manage files';
    paragraph.innerHTML = 'Manage your files here.';

    const table = document.createElement('table');
    table.classList = 'file-table';

    const upload_file = document.createElement('button');
    upload_file.innerText = 'Upload new file';
    upload_file.onclick = () => {
        upload_file_window(data);
    }

    function print_list(data, list) {
        let to_delete = [];
        while (table.firstChild) {
            table.removeChild(table.firstChild);
        }

        if (list.length === 0) {
            const row = document.createElement('tr');
            const cell = document.createElement('td');
            cell.innerText = 'No files found.';
            row.appendChild(cell);
            table.appendChild(row);
            return;
        }

        const check_all = document.createElement('input');
        check_all.type = 'checkbox';
        check_all.id = 'file-all';
        check_all.name = 'file-all';
        check_all.checked = false;
        check_all.onchange = () => {
            const checks = document.getElementsByTagName('input');
            for (let i = 0; i < checks.length; i++) {
                if (checks[i].type === 'checkbox' && checks[i].id !== 'file-all') {
                    checks[i].checked = check_all.checked;
                }
            }

            to_delete = [];
            if (check_all.checked) {
                for (const file of list) {
                    to_delete.push(file);
                }
            }
        }

        const check_all_label = document.createElement('label');
        check_all_label.htmlFor = 'file-all';
        check_all_label.innerText = 'Select all';

        const check_all_cell = document.createElement('td');
        check_all_cell.appendChild(check_all);
        check_all_cell.appendChild(check_all_label);

        const check_all_row = document.createElement('tr');
        check_all_row.appendChild(check_all_cell);

        const delete_checked = document.createElement('button');
        delete_checked.innerText = 'Delete selected';
        delete_checked.onclick = async () => {
            if (to_delete.length === 0) {
                return;
            }

            delete_file_window(data, to_delete);
        }

        check_all_row.appendChild(delete_checked);

        table.appendChild(check_all_row);

        for (const file of list) {
            const row = document.createElement('tr');
            const cell = document.createElement('td');

            const check = document.createElement('input');
            check.type = 'checkbox';
            check.id = 'file-' + file;
            check.name = 'file-' + file;
            check.checked = false;
            check.onchange = () => {
                const checks = document.getElementsByTagName('input');
                let all_checked = true;
                for (let i = 0; i < checks.length; i++) {
                    if (checks[i].type === 'checkbox' && checks[i].id !== 'file-all' && checks[i].checked === false) {
                        all_checked = false;
                        break;
                    }
                }
                check_all.checked = all_checked;

                // add or remove from to_delete
                if (check.checked) {
                    to_delete.push(file);
                } else {
                    const index = to_delete.indexOf(file);
                    if (index > -1) {
                        to_delete.splice(index, 1);
                    }
                }
            }
            cell.appendChild(check);

            const link = document.createElement('a');
            link.onclick = () => {
                manage_file(data, file);
            }

            link.innerText = file;

            cell.appendChild(link);

            const delete_button = document.createElement('button');
            delete_button.innerText = 'Delete';
            delete_button.onclick = async () => {
                delete_file_window(data, [file]);
            }

            cell.appendChild(delete_button);

            row.appendChild(cell);
            table.appendChild(row);
        }
    }

    print_list(data, await get_files());

    const search = document.createElement('input');
    search.placeholder = 'Search';
    search.oninput = async () => {
        const search_results = await get_files();

        // remove entries that don't match the search
        for (let i = 0; i < search_results.length; i++) {
            if (!search_results[i].includes(search.value)) {
                search_results.splice(i, 1);
                i--;
            }
        }

        print_list(data, search_results);
    }

    win.appendChild(title);
    win.appendChild(paragraph);
    win.appendChild(upload_file);
    win.appendChild(document.createElement('br'));
    win.appendChild(document.createElement('br'));
    win.appendChild(search);
    win.appendChild(document.createElement('br'));
    win.appendChild(document.createElement('br'));
    win.appendChild(table);
}

function user_manage(data) {
    hide_all_windows();
}

function server_manage(data) {
    hide_all_windows();
}

function admin(data) {
    const win = create_window('admin-window');

    const title = document.createElement('h1');
    const paragraph = document.createElement('p');

    title.innerText = 'Admin panel';
    title.classList = 'admin-title';
    paragraph.innerText = 'Welcome to the admin panel.';

    const page_management = document.createElement('button');
    page_management.innerText = 'Page management';
    page_management.onclick = () => {
        page_manage(data);
    }

    const file_management = document.createElement('button');
    file_management.innerText = 'File management';
    file_management.onclick = () => {
        file_manage(data);
    }

    const user_management = document.createElement('button');
    user_management.innerText = 'User management';
    user_management.onclick = () => {
        user_manage(data);
    }

    const server_management = document.createElement('button');
    server_management.innerText = 'Server management';
    server_management.onclick = () => {
        server_manage(data);
    }

    win.appendChild(title);
    win.appendChild(paragraph);
    win.appendChild(page_management);
    win.appendChild(file_management);
    win.appendChild(user_management);
    win.appendChild(server_management);
}

function e404(data) {
    set_path('/');
    const win = create_window('404-window');

    const title = document.createElement('h1');
    const paragraph = document.createElement('p');

    title.innerText = '404 - Page not found';
    title.classList = 'error-title';
    paragraph.innerText = 'The page you are looking for does not exist. Perhaps the owner of this site forgot to create it?';
    paragraph.classList = 'error-paragraph';

    // TODO: Some kind of 'perhaps you meant' thing

    win.appendChild(title);
    win.appendChild(paragraph);
}

async function load_page(path, data) {
    hide_all_windows();

    if (path === last_path_page) {
        return; // no need to reload the same crap
    }

    if (path === "/login" && get_cookie('username') === null) {
        login(data);
        reinitialize_content();
        return;
    } else if (path === "/logout") {
        logout(data);
        reinitialize_content();
        return;
    } else if (path === "/register") {
        register(data);
        reinitialize_content();
        return;
    } else if (path === "/admin" && get_cookie('user_type') == 1) {
        admin(data);
        reinitialize_content();
        return;
    }

    if (path === "/admin/page") {
        page_manage(data);
        reinitialize_content();
        return;
    }
    if (path === "/admin/file") {
        file_manage(data);
        reinitialize_content();
        return;
    }

    // /admin/page/...
    if (path.includes('/admin/page/')) {
        const page = path.split('/').pop();
        manage_page(data, page);
        reinitialize_content();
        return;
    } else if (path.includes('/admin/file/')) {
        const file = path.split('/').pop();
        manage_file(data, file);
        reinitialize_content();
        return;
    }

    last_path_page = path;

    // do not move me up please
    const _content = document.getElementById('content');
    if (_content) {
        while (_content.firstChild) {
            _content.removeChild(_content.firstChild);
        }
    }

    async function fetch_page(path) {
        const api = "/api/get_page";
        const send = { page: path };

        try {
            const response = await fetch(api, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(send),
            });

            if (!response.ok) {
                throw new Error('Network response was not ok');
            }

            const json = await response.json();

            if (json.error_str) {
                console.error('Error from server:', json.error_str);
                return undefined;
            }

            if (json.output_content === undefined) {
                console.error('output_content is undefined');
                return undefined;
            }

            if (json.output_content_type !== 'html') {
                throw new Error('Invalid content type: ' + json.output_content_type);
            }

            return json.output_content;
        } catch (error) {
            // could log here if needed for debug
            return undefined;
        }
    }

    const content = await fetch_page(path);
    if (content !== undefined && content !== null) {
        const div = document.getElementById('content');
        div.innerHTML += content;

        /* replace all links with onclick events to load_page
         * if they're already onclick, ignore
         */
        const links = document.getElementsByTagName('a');
        for (const link of links) {
            if (link.onclick === null) {
                link.onclick = (event) => {
                    event.preventDefault();
                    set_path(link.pathname);
                    load_page(link.pathname, data);
                }
            }
        }

        /* find title in div and set it as the document title
         * then remove it from the content
         */
        const title = div.getElementsByTagName('title');
        if (title.length > 0) {
            document.title = title[title.length-1].innerText;
            title[title.length-1].remove();
        }

        /* same for meta tags */
        const meta = div.getElementsByTagName('meta');
        for (const tag of meta) {
            // remove existing from header
            const existing = document.querySelector('meta[name="' + tag.getAttribute('name') + '"]');
            if (existing) {
                existing.remove();
            }

            // add to header
            document.head.appendChild(tag);
        }

        reinitialize_content();
        return;
    } else {
        // external website
        if (path.includes("https://") || path.includes("http://")) {
            window.location = path;
            return;
        }
        // no?
        // 404
        reinitialize_content();
        return e404(data);
    }
}

function append_footer(data) {
    const footer = document.createElement('footer');
    footer.id = 'footer';
    footer.classList.add('footer');

    const p = document.createElement('p');
    p.innerText = data.footer.text;
    footer.appendChild(p);

    document.documentElement.appendChild(footer);
}

function append_header(data) {
    const div = document.createElement('div');
    div.id = 'topBar';
    div.classList.add('top-bar');

    const create_div = (data, dark) => {
        const link_div = document.createElement('div');
        link_div.classList = 'header ' + (dark ? 'dark' : 'light');

        const link_a = document.createElement('a');
        link_a.href = '/';
        link_a.id = 'title';
        link_a.innerText = data.header.title;

        if (data.header.logo !== undefined) {
            const link_img = document.createElement('img');
            link_img.src = data.header.logo;
            link_img.id = 'logo';
            link_img.style.height = "100px";
            link_img.style.width = "100px";
            link_img.style.verticalAlign = "middle";
            link_img.style.padding = "10px";

            if ((dark && data.header.light_logo !== undefined && data.header.light_logo)
                || (!dark && data.header.light_logo !== undefined && !data.header.light_logo)) {
                link_img.style.filter = "invert(1)";
            }

            link_a.innerHTML = '';
            link_a.appendChild(link_img);
            link_a.appendChild(document.createTextNode(data.header.title));
        }

        link_div.appendChild(link_a);
        div.appendChild(link_div);
    }

    create_div(data, false);
    create_div(data, true);

    if (data.header.links !== undefined) {
        const nav = document.createElement("div");
        nav.classList.add('nav-links');
        nav.id = 'navLinks';

        let logged_in = false;
        let admin = false;

        if (get_cookie('username') !== null && get_cookie('user_type') !== null) {
            logged_in = true;
        }
        if (get_cookie('user_type') == 1) {
            admin = true;
        }

        for (const link of data.header.links) {
            if ((link.auth !== undefined && link.auth) && !logged_in) {
                continue;
            } else if ((link.admin !== undefined && link.admin) && !admin) {
                continue;
            } else if ((link.auth !== undefined && !link.auth) && logged_in) {
                continue;
            } else if ((link.admin !== undefined && !link.admin) && admin) {
                continue;
            }

            const a = document.createElement('a');
            a.innerText = link.name;
            a.onclick = (event) => {
                set_path(link.path);
                load_page(link.path, data);
            }
            if (link.id !== undefined) {
                a.id = link.id;
            }
            if (link.class !== undefined) {
                a.classList.add(link.class);
            }
            if (link.href !== undefined) {
                a.href = link.href;
            }
            nav.appendChild(a);
        }

        div.appendChild(nav);
    }

    document.body.appendChild(div);
}

function set_defaults(data) {
    document.title = data.default_site.title;

    const meta_description = document.createElement('meta');
    meta_description.setAttribute('name', 'description');
    meta_description.content = data.default_site.description;
    document.head.appendChild(meta_description);

    const meta_keywords = document.createElement('meta');
    meta_keywords.setAttribute('name', 'keywords');
    meta_keywords.content = data.default_site.keywords;
    document.head.appendChild(meta_keywords);

    const meta_author = document.createElement('meta');
    meta_author.setAttribute('name', 'author');
    meta_author.content = data.default_site.author;
    document.head.appendChild(meta_author);

    const meta_viewport = document.createElement('meta');
    meta_viewport.setAttribute('name', 'viewport');
    meta_viewport.content = data.default_site.viewport;
    document.head.appendChild(meta_viewport);

    const meta_charset = document.createElement('meta');
    meta_charset.setAttribute('charset', 'UTF-8');
    document.head.appendChild(meta_charset);

    if (data.default_site.enable_opengraph !== undefined && data.default_site.enable_opengraph) {
        // Open Graph
        const meta_og_title = document.createElement('meta');
        meta_og_title.setAttribute('property', 'og:title');
        meta_og_title.setAttribute('content', data.default_site.title);
        document.head.appendChild(meta_og_title);

        const meta_og_description = document.createElement('meta');
        meta_og_description.setAttribute('property', 'og:description');
        meta_og_description.setAttribute('content', data.default_site.description);
        document.head.appendChild(meta_og_description);

        const meta_og_type = document.createElement('meta');
        meta_og_type.setAttribute('property', 'og:type');
        meta_og_type.setAttribute('content', 'website');
        document.head.appendChild(meta_og_type);

        const meta_og_url = document.createElement('meta');
        meta_og_url.setAttribute('property', 'og:url');
        meta_og_url.setAttribute('content', window.location.href);
        document.head.appendChild(meta_og_url);
    }

    const link_favicon = document.createElement('link');
    link_favicon.rel = 'icon';
    link_favicon.href = data.default_site.favicon;
    document.head.appendChild(link_favicon);

    const css = document.createElement('link');
    css.rel = 'stylesheet';
    css.href = '/css/main.css';
    document.head.appendChild(css);
}

function reinitialize_content() {
    const footer = document.querySelector('footer');
    const body = document.body;
    const nav_bar = document.querySelector('.top-bar');

    if (document.body.scrollHeight <= window.innerHeight && footer) {
        footer.classList.add('visible');
    }

    function set_padding() {
        if (!footer) {
            return;
        }
        const fh = footer.offsetHeight;
        body.style.paddingBottom = `${fh}px`;
    }

    function set_nav_bar_padding() {
        if (!nav_bar) {
            return;
        }
        const nh = nav_bar.offsetHeight;
        body.style.paddingTop = `${nh}px`;
    }

    set_padding();
    set_nav_bar_padding();
    window.addEventListener('resize', set_padding);
    window.addEventListener('resize', set_nav_bar_padding);

    window.addEventListener('scroll', function () {
        const topBar = document.getElementById('topBar');
        const footer = document.getElementById('footer');

        if (window.scrollY > 50) {
            topBar.classList.add('hidden');
        } else {
            topBar.classList.remove('hidden');
        }

        if (window.scrollY + window.innerHeight >= document.body.scrollHeight - 50) {
            footer.classList.add('visible');
        } else {
            footer.classList.remove('visible');
        }
    });
}

window.addEventListener('load', function() {
    reinitialize_content();
});

function main(data) {
    set_defaults(data);
    append_footer(data);
    append_header(data);

    const div = document.createElement('div');
    div.classList = 'content';
    div.id = 'content';

    document.body.appendChild(div);

    load_page(get_path(), data);
}

document.addEventListener('DOMContentLoaded', function() {
    include("https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.9.0/highlight.min.js");
    include("https://kit.fontawesome.com/aa55cd1c33.js");

    get_site_settings().then((data) => {
        main(data);
    });
})